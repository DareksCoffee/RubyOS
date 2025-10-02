#include "rfss.h"
#include "../drivers/ata.h"
#include "../mm/memory.h"
#include "../kernel/logger.h"
#include <string.h>

static rfss_inode_t* rfss_get_inode(rfss_fs_t* fs, uint32_t inode_num) {
    if (!fs || inode_num == 0 || inode_num > fs->superblock->inode_count) {
        //log(LOG_ERROR, "Invalid inode number: %d", inode_num);
        return NULL;
    }
    
    uint32_t inodes_per_block = RFSS_BLOCK_SIZE / sizeof(rfss_inode_t);
    uint32_t block = (inode_num - 1) / inodes_per_block;
    uint32_t index = (inode_num - 1) % inodes_per_block;
    
    static rfss_inode_t inodes[RFSS_BLOCK_SIZE / sizeof(rfss_inode_t)];
    if (rfss_read_block(fs, fs->superblock->inode_table_block + block, inodes) != 0) {
        //log(LOG_ERROR, "Failed to read inode block %d", block);
        return NULL;
    }
    
    return &inodes[index];
}

static int rfss_write_inode(rfss_fs_t* fs, uint32_t inode_num, rfss_inode_t* inode) {
    if (!fs || !inode || inode_num == 0 || inode_num > fs->superblock->inode_count) {
        //log(LOG_ERROR, "Invalid parameters for write inode");
        return -1;
    }

    if (fs->journaling_enabled) {
        return rfss_safe_write_inode(fs, inode_num, inode);
    }
    
    uint32_t inodes_per_block = RFSS_BLOCK_SIZE / sizeof(rfss_inode_t);
    uint32_t block = (inode_num - 1) / inodes_per_block;
    uint32_t index = (inode_num - 1) % inodes_per_block;

    static rfss_inode_t inodes[RFSS_BLOCK_SIZE / sizeof(rfss_inode_t)];
    if (rfss_read_block(fs, fs->superblock->inode_table_block + block, inodes) != 0) {
        //log(LOG_ERROR, "Failed to read inode block for writing");
        return -1;
    }
    
    memcpy(&inodes[index], inode, sizeof(rfss_inode_t));
    int result = rfss_write_block(fs, fs->superblock->inode_table_block + block, inodes);
    if (result != 0) {
       //log(LOG_ERROR, "Failed to write inode block");
    }
    return result;
}

static uint32_t rfss_find_file_in_directory(rfss_fs_t* fs, uint32_t dir_inode, const char* name) {
    if (!fs || !name || strlen(name) == 0 || strlen(name) >= RFSS_MAX_FILENAME) {
        //log(LOG_ERROR, "Invalid filename");
        return 0;
    }
    
    rfss_inode_t* inode = rfss_get_inode(fs, dir_inode);
    if (!inode || ((inode->mode >> 12) & 0xF) != RFSS_FILE_DIRECTORY) {
        //log(LOG_ERROR, "Not a directory inode: %d", dir_inode);
        return 0;
    }

    static uint8_t block_buffer[RFSS_BLOCK_SIZE];
    for (int i = 0; i < RFSS_DIRECT_BLOCKS && inode->direct_blocks[i]; i++) {
        if (rfss_read_block(fs, inode->direct_blocks[i], block_buffer) != 0) {
            continue;
        }
        
        uint32_t offset = 0;
        while (offset < RFSS_BLOCK_SIZE) {
            rfss_dir_entry_t* entry = (rfss_dir_entry_t*)(block_buffer + offset);
            if (entry->rec_len == 0 || entry->rec_len > RFSS_BLOCK_SIZE - offset || 
                entry->rec_len < sizeof(rfss_dir_entry_t)) {
                break;
            }
            
            if (entry->inode != 0 && entry->name_len > 0 && 
                entry->name_len < RFSS_MAX_FILENAME && 
                entry->name_len == strlen(name) && 
                memcmp(entry->name, name, entry->name_len) == 0) {
                return entry->inode;
            }
            
            offset += entry->rec_len;
        }
    }
    
    return 0;
}

static uint32_t rfss_resolve_path(rfss_fs_t* fs, const char* path) {
    if (!fs || !path) {
        //log(LOG_ERROR, "Invalid parameters for path resolution");
        return 0;
    }
    
    uint32_t current_inode;
    if (path[0] == '/') {
        current_inode = fs->superblock->root_inode;
        path++;
    } else {
        current_inode = fs->current_dir_inode;
    }
    
    if (*path == '\0') {
        return current_inode;
    }
    
    char component[RFSS_MAX_FILENAME + 1];
    while (*path) {
        int i = 0;
        while (*path && *path != '/' && i < RFSS_MAX_FILENAME) {
            component[i++] = *path++;
        }
        component[i] = '\0';
        
        if (*path == '/') path++;
        
        if (strlen(component) == 0) continue;
        
        current_inode = rfss_find_file_in_directory(fs, current_inode, component);
        if (current_inode == 0) {
            //log(LOG_ERROR, "Path component not found: %s", component);
            return 0;
        }
    }
    
    return current_inode;
}

static int rfss_add_directory_entry(rfss_fs_t* fs, uint32_t dir_inode, const char* name, uint32_t file_inode, uint8_t file_type) {
    if (!fs || !name || strlen(name) == 0 || strlen(name) >= RFSS_MAX_FILENAME || file_inode == 0) {
        //log(LOG_ERROR, "Invalid parameters for add directory entry");
        return -1;
    }
    
    rfss_inode_t* inode = rfss_get_inode(fs, dir_inode);
    if (!inode || ((inode->mode >> 12) & 0xF) != RFSS_FILE_DIRECTORY) {
        //log(LOG_ERROR, "Not a directory for adding entry");
        return -1;
    }
    
    uint16_t name_len = strlen(name);
    uint16_t entry_size = sizeof(rfss_dir_entry_t) + name_len;
    entry_size = (entry_size + 3) & ~3;
    
    if (entry_size > RFSS_BLOCK_SIZE - sizeof(rfss_dir_entry_t)) {
        //log(LOG_ERROR, "Filename too long");
        return -1;
    }
    
    static uint8_t block_buffer[RFSS_BLOCK_SIZE];

    for (int i = 0; i < RFSS_DIRECT_BLOCKS && inode->direct_blocks[i]; i++) {
        if (rfss_read_block(fs, inode->direct_blocks[i], block_buffer) != 0) {
            continue;
        }
        
        uint32_t offset = 0;
        rfss_dir_entry_t* last_entry = NULL;
        uint32_t last_offset = 0;
        
        while (offset < RFSS_BLOCK_SIZE) {
            rfss_dir_entry_t* entry = (rfss_dir_entry_t*)(block_buffer + offset);
            if (entry->rec_len == 0 || entry->rec_len > RFSS_BLOCK_SIZE - offset ||
                entry->rec_len < sizeof(rfss_dir_entry_t)) {
                break;
            }
            
            last_entry = entry;
            last_offset = offset;
            offset += entry->rec_len;
        }
        
        if (last_entry && last_entry->name_len < RFSS_MAX_FILENAME) {
            uint32_t actual_last_size = sizeof(rfss_dir_entry_t) + last_entry->name_len;
            actual_last_size = (actual_last_size + 3) & ~3;
            
            uint32_t free_space = RFSS_BLOCK_SIZE - (last_offset + actual_last_size);
            
            if (free_space >= entry_size) {
                last_entry->rec_len = actual_last_size;
                
                rfss_dir_entry_t* new_entry = (rfss_dir_entry_t*)(block_buffer + last_offset + actual_last_size);
                new_entry->inode = file_inode;
                new_entry->rec_len = free_space;
                new_entry->name_len = name_len;
                new_entry->file_type = file_type;
                memcpy(new_entry->name, name, name_len);
                memset(new_entry->name + name_len, 0, RFSS_MAX_FILENAME - name_len);
                
                if (rfss_write_block(fs, inode->direct_blocks[i], block_buffer) == 0) {
                    //log(LOG_DEBUG, "Added directory entry '%s' to existing block", name);
                    return 0;
                } else {
                    //log(LOG_ERROR, "Failed to write directory block");
                    return -1;
                }
            }
        }
    }
    
    for (int i = 0; i < RFSS_DIRECT_BLOCKS; i++) {
        if (inode->direct_blocks[i] == 0) {
            uint32_t new_block = rfss_allocate_block(fs);
            if (new_block == 0) {
                //log(LOG_ERROR, "Failed to allocate block for directory");
                return -1;
            }
            
            inode->direct_blocks[i] = new_block;
            inode->blocks_count++;
            inode->size += RFSS_BLOCK_SIZE;
            
            memset(block_buffer, 0, RFSS_BLOCK_SIZE);
            
            rfss_dir_entry_t* new_entry = (rfss_dir_entry_t*)block_buffer;
            new_entry->inode = file_inode;
            new_entry->rec_len = RFSS_BLOCK_SIZE;
            new_entry->name_len = name_len;
            new_entry->file_type = file_type;
            memcpy(new_entry->name, name, name_len);
            memset(new_entry->name + name_len, 0, RFSS_MAX_FILENAME - name_len);
            
            if (rfss_write_block(fs, new_block, block_buffer) != 0) {
                //log(LOG_ERROR, "Failed to write new directory block");
                rfss_free_block(fs, new_block);
                return -1;
            }
            
            if (rfss_write_inode(fs, dir_inode, inode) != 0) {
                //log(LOG_ERROR, "Failed to update directory inode");
                rfss_free_block(fs, new_block);
                return -1;
            }
            
            //log(LOG_DEBUG, "Added directory entry '%s' to new block %d", name, new_block);
            return 0;
        }
    }
    
    //log(LOG_ERROR, "Directory is full, no more direct blocks available");
    return -1;
}

int rfss_create_file(rfss_fs_t* fs, const char* path, uint32_t mode) {
    if (!fs || !path || !fs->mounted || strlen(path) == 0 || strlen(path) > 255) {
        //log(LOG_ERROR, "Invalid parameters for create file");
        return -1;
    }
    
    char* path_copy = kmalloc(strlen(path) + 1);
    if (!path_copy) {
        //log(LOG_ERROR, "Memory allocation failed");
        return -1;
    }
    strcpy(path_copy, path);
    
    int original_absolute = (path[0] == '/') ? 1 : 0;
    char* last_slash = strrchr(path_copy, '/');
    char* filename;
    char* parent_path_str;
    if (last_slash) {
        *last_slash = '\0';
        filename = last_slash + 1;
        parent_path_str = path_copy;
    } else {
        filename = path_copy;
        parent_path_str = NULL;
    }
    
    if (strlen(filename) == 0 || strlen(filename) >= RFSS_MAX_FILENAME) {
        //log(LOG_ERROR, "Invalid filename");
        kfree(path_copy);
        return -1;
    }
    
    uint32_t parent_inode;
    if (!parent_path_str || strlen(parent_path_str) == 0) {
        if (original_absolute) {
            parent_inode = fs->superblock->root_inode;
        } else {
            parent_inode = fs->current_dir_inode;
        }
    } else {
        parent_inode = rfss_resolve_path(fs, parent_path_str);
        if (parent_inode == 0) {
            //log(LOG_ERROR, "Parent directory not found");
            kfree(path_copy);
            return -1;
        }
    }
    
    if (rfss_find_file_in_directory(fs, parent_inode, filename) != 0) {
        //log(LOG_ERROR, "File already exists: %s", filename);
        kfree(path_copy);
        return -1;
    }
    
    uint32_t new_inode_num = rfss_allocate_inode(fs);
    if (new_inode_num == 0) {
        //log(LOG_ERROR, "Failed to allocate inode");
        kfree(path_copy);
        return -1;
    }
    
    rfss_inode_t new_inode;
    memset(&new_inode, 0, sizeof(rfss_inode_t));
    new_inode.mode = mode | (RFSS_FILE_REGULAR << 12);
    new_inode.uid = 0;
    new_inode.gid = 0;
    new_inode.size = 0;
    new_inode.links_count = 1;
    new_inode.blocks_count = 0;
    
    if (rfss_write_inode(fs, new_inode_num, &new_inode) != 0) {
        //log(LOG_ERROR, "Failed to write new inode");
        rfss_free_inode(fs, new_inode_num);
        kfree(path_copy);
        return -1;
    }
    
    if (rfss_add_directory_entry(fs, parent_inode, filename, new_inode_num, RFSS_FILE_REGULAR) != 0) {
        //log(LOG_ERROR, "Failed to add directory entry");
        rfss_free_inode(fs, new_inode_num);
        kfree(path_copy);
        return -1;
    }
    
    log(LOG_DEBUG, "Created file: %s (inode %d)", path, new_inode_num);
    kfree(path_copy);
    return 0;
}

static rfss_fs_t* mounted_fs = NULL;

uint32_t rfss_calculate_checksum(const void* data, size_t size) {
    if (!data) {
        return 0;
    }
    uint32_t checksum = 0;
    const uint8_t* ptr = (const uint8_t*)data;
    for (size_t i = 0; i < size; i++) {
        checksum = (checksum << 5) + checksum + ptr[i];
    }
    return checksum;
}

int rfss_read_block(rfss_fs_t* fs, uint32_t block, void* buffer) {
    if (!buffer) {
        //log(LOG_ERROR, "Invalid buffer for read block");
        return -1;
    }

    uint32_t device_id = fs ? fs->device_id : 0;

    if (fs && fs->superblock && block >= fs->superblock->total_blocks && block != 0) {
        //log(LOG_ERROR, "Block number out of range: %d >= %d", block, fs->superblock->total_blocks);
        return -1;
    }

    uint32_t sectors_per_block = RFSS_BLOCK_SIZE / ATA_SECTOR_SIZE;
    uint32_t start_sector = block * sectors_per_block;

    for (uint32_t i = 0; i < sectors_per_block; i++) {
        if (ata_read_sectors(device_id, start_sector + i, 1, (uint8_t*)buffer + i * ATA_SECTOR_SIZE) != 0) {
            //log(LOG_ERROR, "Failed to read sector %d", start_sector + i);
            return -1;
        }
    }

    return 0;
}

int rfss_write_block(rfss_fs_t* fs, uint32_t block, const void* buffer) {
    if (!buffer) {
        //log(LOG_ERROR, "Invalid buffer for write block");
        return -1;
    }

    if (fs && fs->journaling_enabled) {
        return rfss_safe_write_block(fs, block, buffer);
    }

    uint32_t device_id = fs ? fs->device_id : 0;

    if (fs && fs->superblock && block >= fs->superblock->total_blocks && block != 0) {
        //log(LOG_ERROR, "Block number out of range: %d >= %d", block, fs->superblock->total_blocks);
        return -1;
    }

    uint32_t sectors_per_block = RFSS_BLOCK_SIZE / ATA_SECTOR_SIZE;
    uint32_t start_sector = block * sectors_per_block;

    for (uint32_t i = 0; i < sectors_per_block; i++) {
        if (ata_write_sectors(device_id, start_sector + i, 1, (const uint8_t*)buffer + i * ATA_SECTOR_SIZE) != 0) {
            //log(LOG_ERROR, "Failed to write sector %d", start_sector + i);
            return -1;
        }
    }

    return 0;
}

uint32_t rfss_allocate_block(rfss_fs_t* fs) {
    if (!fs || !fs->block_bitmap || !fs->superblock) {
        //log(LOG_ERROR, "Filesystem not properly initialized");
        return 0;
    }

    if (fs->superblock->free_blocks == 0) {
        //log(LOG_ERROR, "No free blocks available");
        return 0;
    }

    uint32_t total_blocks = fs->superblock->total_blocks;
    for (uint32_t i = 80; i < total_blocks; i++) {
        uint32_t byte = i / 8;
        uint32_t bit = i % 8;
        if (!(fs->block_bitmap[byte] & (1 << bit))) {
            fs->block_bitmap[byte] |= (1 << bit);
            fs->superblock->free_blocks--;
            fs->dirty = 1;
            return i;
        }
    }

    //log(LOG_ERROR, "No free blocks available");
    return 0;
}

void rfss_free_block(rfss_fs_t* fs, uint32_t block) {
    if (!fs || !fs->block_bitmap || block >= fs->superblock->total_blocks || block < 80) {
        return;
    }

    uint32_t byte = block / 8;
    uint32_t bit = block % 8;
    if (fs->block_bitmap[byte] & (1 << bit)) {
        fs->block_bitmap[byte] &= ~(1 << bit);
        fs->superblock->free_blocks++;
        fs->dirty = 1;
    }
}

uint32_t rfss_allocate_inode(rfss_fs_t* fs) {
    if (!fs || !fs->inode_bitmap || !fs->superblock) {
        //log(LOG_ERROR, "Filesystem not properly initialized");
        return 0;
    }

    if (fs->superblock->free_inode_count == 0) {
        //log(LOG_ERROR, "No free inodes available");
        return 0;
    }

    uint32_t total_inodes = fs->superblock->inode_count;
    for (uint32_t i = 1; i <= total_inodes; i++) {
        uint32_t byte = (i - 1) / 8;
        uint32_t bit = (i - 1) % 8;
        if (!(fs->inode_bitmap[byte] & (1 << bit))) {
            fs->inode_bitmap[byte] |= (1 << bit);
            fs->superblock->free_inode_count--;
            fs->dirty = 1;
            return i;
        }
    }

    //log(LOG_ERROR, "No free inodes available");
    return 0;
}

void rfss_free_inode(rfss_fs_t* fs, uint32_t inode) {
    if (!fs || !fs->inode_bitmap || inode == 0 || inode > fs->superblock->inode_count) {
        return;
    }

    uint32_t byte = (inode - 1) / 8;
    uint32_t bit = (inode - 1) % 8;
    if (fs->inode_bitmap[byte] & (1 << bit)) {
        fs->inode_bitmap[byte] &= ~(1 << bit);
        fs->superblock->free_inode_count++;
        fs->dirty = 1;
    }
}

int rfss_format(uint32_t device_id, const char* label) {
    //log(LOG_DEBUG, "rfss_format: device_id=%d, label='%s'", device_id, label ? label : "unlabeled");
    if (label && strlen(label) >= 16) {
        //log(LOG_ERROR, "Label too long (max 15 characters)");
        return -1;
    }

    if (!ata_drive_exists(device_id)) {
        //log(LOG_ERROR, "Drive %d does not exist", device_id);
        return -1;
    }

    uint32_t drive_size = ata_get_drive_size(device_id);
    uint32_t total_sectors = drive_size;
    uint32_t sectors_per_block = RFSS_BLOCK_SIZE / ATA_SECTOR_SIZE;
    uint32_t total_blocks = total_sectors / sectors_per_block;

    if (total_blocks < 81) {
        //log(LOG_ERROR, "Drive too small for filesystem (need at least 81 blocks)");
        return -1;
    }

    rfss_superblock_t superblock;
    memset(&superblock, 0, sizeof(rfss_superblock_t));
    superblock.magic = RFSS_MAGIC;
    superblock.version = RFSS_VERSION;
    superblock.block_size = RFSS_BLOCK_SIZE;
    superblock.total_blocks = total_blocks;
    uint32_t reserved_blocks = 80;
    superblock.free_blocks = total_blocks - reserved_blocks;
    superblock.inode_table_block = 1;
    superblock.inode_count = 1024;
    superblock.free_inode_count = 1023;
    superblock.root_inode = 1;
    superblock.bitmap_block = 69;
    superblock.journal_block = 71;
    superblock.journal_size = 0;
    superblock.created_time = 0;
    superblock.modified_time = 0;
    superblock.mount_count = 0;
    superblock.max_mount_count = 100;
    superblock.state = 1;
    superblock.errors = 0;
    memset(superblock.uuid, 0, 16);
    if (label) {
        strncpy(superblock.label, label, 15);
        superblock.label[15] = '\0';
    } else {
        strcpy(superblock.label, "RFSS_DRIVE");
    }
    memset(superblock.reserved, 0, sizeof(superblock.reserved));
    //log(LOG_DEBUG, "Superblock: magic=0x%x, total_blocks=%d, free_blocks=%d", superblock.magic, superblock.total_blocks, superblock.free_blocks);

    if (rfss_write_block(NULL, 0, &superblock) != 0) {
        //log(LOG_ERROR, "Failed to write superblock");
        return -1;
    }

    //log(LOG_DEBUG, "Initializing bitmaps");
    uint32_t bitmap_size = (total_blocks + 7) / 8;
    //log(LOG_DEBUG, "bitmap_size=%d", bitmap_size);
    uint8_t* block_bitmap = kmalloc(RFSS_BLOCK_SIZE);
    if (!block_bitmap) {
        //log(LOG_ERROR, "Failed to allocate block bitmap");
        return -1;
    }
    memset(block_bitmap, 0, RFSS_BLOCK_SIZE);

    for (uint32_t i = 0; i < reserved_blocks && i < total_blocks; i++) {
        uint32_t byte = i / 8;
        uint32_t bit = i % 8;
        if (byte < RFSS_BLOCK_SIZE) {
            block_bitmap[byte] |= (1 << bit);
        }
    }

    if (rfss_write_block(NULL, superblock.bitmap_block, block_bitmap) != 0) {
        //log(LOG_ERROR, "Failed to write block bitmap");
        kfree(block_bitmap);
        return -1;
    }

    uint8_t* inode_bitmap = kmalloc(RFSS_BLOCK_SIZE);
    if (!inode_bitmap) {
        //log(LOG_ERROR, "Failed to allocate inode bitmap");
        kfree(block_bitmap);
        return -1;
    }
    memset(inode_bitmap, 0, RFSS_BLOCK_SIZE);
    inode_bitmap[0] |= 1;

    uint32_t inode_bitmap_block = superblock.bitmap_block + 1;
    if (rfss_write_block(NULL, inode_bitmap_block, inode_bitmap) != 0) {
        //log(LOG_ERROR, "Failed to write inode bitmap");
        kfree(block_bitmap);
        kfree(inode_bitmap);
        return -1;
    }

    //log(LOG_DEBUG, "Initializing inode table");
    uint32_t inodes_per_block = RFSS_BLOCK_SIZE / sizeof(rfss_inode_t);
    uint32_t inode_blocks = (superblock.inode_count + inodes_per_block - 1) / inodes_per_block;
    rfss_inode_t* inode_table = kmalloc(inode_blocks * RFSS_BLOCK_SIZE);
    if (!inode_table) {
        //log(LOG_ERROR, "Failed to allocate inode table");
        kfree(block_bitmap);
        kfree(inode_bitmap);
        return -1;
    }
    memset(inode_table, 0, inode_blocks * RFSS_BLOCK_SIZE);

    inode_table[0].mode = 0755 | (RFSS_FILE_DIRECTORY << 12);
    inode_table[0].uid = 0;
    inode_table[0].gid = 0;
    inode_table[0].size = RFSS_BLOCK_SIZE;
    inode_table[0].links_count = 2;
    inode_table[0].blocks_count = 1;
    inode_table[0].direct_blocks[0] = 79;

    for (uint32_t i = 0; i < inode_blocks; i++) {
        if (rfss_write_block(NULL, superblock.inode_table_block + i, (uint8_t*)inode_table + i * RFSS_BLOCK_SIZE) != 0) {
            kfree(block_bitmap);
            kfree(inode_bitmap);
            kfree(inode_table);
            return -1;
        }
    }

    static uint8_t root_block[RFSS_BLOCK_SIZE];
    memset(root_block, 0, RFSS_BLOCK_SIZE);

    rfss_dir_entry_t* dot_entry = (rfss_dir_entry_t*)root_block;
    dot_entry->inode = 1;
    dot_entry->rec_len = 12;
    dot_entry->name_len = 1;
    dot_entry->file_type = RFSS_FILE_DIRECTORY;
    dot_entry->name[0] = '.';
    memset(&dot_entry->name[1], 0, RFSS_MAX_FILENAME - 1);

    rfss_dir_entry_t* dotdot_entry = (rfss_dir_entry_t*)(root_block + 12);
    dotdot_entry->inode = 1;
    dotdot_entry->rec_len = RFSS_BLOCK_SIZE - 12;
    dotdot_entry->name_len = 2;
    dotdot_entry->file_type = RFSS_FILE_DIRECTORY;
    dotdot_entry->name[0] = '.';
    dotdot_entry->name[1] = '.';
    memset(&dotdot_entry->name[2], 0, RFSS_MAX_FILENAME - 2);

    if (rfss_write_block(NULL, 79, root_block) != 0) {
        //log(LOG_ERROR, "Failed to write root directory block");
        kfree(block_bitmap);
        kfree(inode_bitmap);
        kfree(inode_table);
        return -1;
    }

    kfree(block_bitmap);
    kfree(inode_bitmap);
    kfree(inode_table);

    log(LOG_OK, "Filesystem formatted successfully on drive %d", device_id);
    return 0;
}

int rfss_mount(uint32_t device_id, rfss_fs_t* fs) {
    if (!fs || mounted_fs) {
        //log(LOG_ERROR, "Invalid mount parameters or filesystem already mounted");
        return -1;
    }

    if (!ata_drive_exists(device_id)) {
        //log(LOG_ERROR, "Drive %d does not exist", device_id);
        return -1;
    }

    memset(fs, 0, sizeof(rfss_fs_t));

    fs->superblock = kmalloc(sizeof(rfss_superblock_t));
    if (!fs->superblock) {
        //log(LOG_ERROR, "Failed to allocate superblock");
        return -1;
    }
    memset(fs->superblock, 0, sizeof(rfss_superblock_t));

    if (rfss_read_block(fs, 0, fs->superblock) != 0) {
        //log(LOG_ERROR, "Failed to read superblock");
        kfree(fs->superblock);
        return -1;
    }

    if (fs->superblock->magic != RFSS_MAGIC) {
        //log(LOG_ERROR, "Invalid filesystem magic: 0x%x", fs->superblock->magic);
        kfree(fs->superblock);
        return -1;
    }

    if (fs->superblock->version != RFSS_VERSION) {
        //log(LOG_ERROR, "Unsupported filesystem version: %d", fs->superblock->version);
        kfree(fs->superblock);
        return -1;
    }

    uint32_t bitmap_size = (fs->superblock->total_blocks + 7) / 8;
    if (bitmap_size > RFSS_BLOCK_SIZE) {
        bitmap_size = RFSS_BLOCK_SIZE;
    }
    fs->block_bitmap = kmalloc(RFSS_BLOCK_SIZE);
    if (!fs->block_bitmap) {
        //log(LOG_ERROR, "Failed to allocate block bitmap");
        kfree(fs->superblock);
        return -1;
    }

    if (rfss_read_block(fs, fs->superblock->bitmap_block, fs->block_bitmap) != 0) {
        //log(LOG_ERROR, "Failed to read block bitmap");
        kfree(fs->superblock);
        kfree(fs->block_bitmap);
        return -1;
    }

    uint32_t inode_bitmap_size = (fs->superblock->inode_count + 7) / 8;
    if (inode_bitmap_size > RFSS_BLOCK_SIZE) {
        inode_bitmap_size = RFSS_BLOCK_SIZE;
    }
    fs->inode_bitmap = kmalloc(RFSS_BLOCK_SIZE);
    if (!fs->inode_bitmap) {
        //log(LOG_ERROR, "Failed to allocate inode bitmap");
        kfree(fs->superblock);
        kfree(fs->block_bitmap);
        return -1;
    }

    if (rfss_read_block(fs, fs->superblock->bitmap_block + 1, fs->inode_bitmap) != 0) {
        //log(LOG_ERROR, "Failed to read inode bitmap");
        kfree(fs->superblock);
        kfree(fs->block_bitmap);
        kfree(fs->inode_bitmap);
        return -1;
    }

    uint32_t inodes_per_block = RFSS_BLOCK_SIZE / sizeof(rfss_inode_t);
    uint32_t inode_blocks = (fs->superblock->inode_count + inodes_per_block - 1) / inodes_per_block;
    fs->inode_table = kmalloc(inode_blocks * RFSS_BLOCK_SIZE);
    if (!fs->inode_table) {
        //log(LOG_ERROR, "Failed to allocate inode table");
        kfree(fs->superblock);
        kfree(fs->block_bitmap);
        kfree(fs->inode_bitmap);
        return -1;
    }

    for (uint32_t i = 0; i < inode_blocks; i++) {
        if (rfss_read_block(fs, fs->superblock->inode_table_block + i, (uint8_t*)fs->inode_table + i * RFSS_BLOCK_SIZE) != 0) {
            //log(LOG_ERROR, "Failed to read inode table block %d", i);
            kfree(fs->superblock);
            kfree(fs->block_bitmap);
            kfree(fs->inode_bitmap);
            kfree(fs->inode_table);
            return -1;
        }
    }

    fs->current_dir_inode = fs->superblock->root_inode;
    strcpy(fs->current_path, "/");
    fs->device_id = device_id;
    fs->mounted = 1;
    fs->dirty = 0;
    fs->journaling_enabled = (fs->superblock->journal_size > 0);
    if (fs->journaling_enabled) {
        rfss_journal_init(fs);
    }

    mounted_fs = fs;

    log(LOG_OK, "Filesystem mounted successfully");
    return 0;
}

int rfss_unmount(rfss_fs_t* fs) {
    if (!fs || !fs->mounted) {
        return -1;
    }

    if (fs->dirty) {
        if (rfss_write_block(fs, 0, fs->superblock) != 0) {
            //log(LOG_ERROR, "Failed to write back superblock");
        }

        if (rfss_write_block(fs, fs->superblock->bitmap_block, fs->block_bitmap) != 0) {
            //log(LOG_ERROR, "Failed to write back block bitmap");
        }

        if (rfss_write_block(fs, fs->superblock->bitmap_block + 1, fs->inode_bitmap) != 0) {
            //log(LOG_ERROR, "Failed to write back inode bitmap");
        }

        uint32_t inodes_per_block = RFSS_BLOCK_SIZE / sizeof(rfss_inode_t);
        uint32_t inode_blocks = (fs->superblock->inode_count + inodes_per_block - 1) / inodes_per_block;
        for (uint32_t i = 0; i < inode_blocks; i++) {
            if (rfss_write_block(fs, fs->superblock->inode_table_block + i, (uint8_t*)fs->inode_table + i * RFSS_BLOCK_SIZE) != 0) {
                //log(LOG_ERROR, "Failed to write back inode table block %d", i);
            }
        }
    }

    kfree(fs->superblock);
    kfree(fs->block_bitmap);
    kfree(fs->inode_bitmap);
    kfree(fs->inode_table);

    memset(fs, 0, sizeof(rfss_fs_t));
    mounted_fs = NULL;

    log(LOG_OK, "Filesystem unmounted successfully");
    return 0;
}

int rfss_delete_file(rfss_fs_t* fs, const char* path) {
    if (!fs || !path || !fs->mounted) {
        return -1;
    }

    uint32_t inode_num = rfss_resolve_path(fs, path);
    if (inode_num == 0) {
        //log(LOG_ERROR, "File not found: %s", path);
        return -1;
    }

    rfss_inode_t* inode = rfss_get_inode(fs, inode_num);
    if (!inode) {
        return -1;
    }

    if (((inode->mode >> 12) & 0xF) == RFSS_FILE_DIRECTORY) {
        //log(LOG_ERROR, "Cannot delete directory with delete_file");
        return -1;
    }

    for (int i = 0; i < RFSS_DIRECT_BLOCKS && inode->direct_blocks[i]; i++) {
        rfss_free_block(fs, inode->direct_blocks[i]);
    }

    rfss_free_inode(fs, inode_num);

    char* path_copy = kmalloc(strlen(path) + 1);
    if (!path_copy) {
        return -1;
    }
    strcpy(path_copy, path);

    char* filename = strrchr(path_copy, '/');
    if (filename) {
        *filename = '\0';
        filename++;
    } else {
        filename = path_copy;
        path_copy = "/";
    }

    uint32_t parent_inode = rfss_resolve_path(fs, path_copy);
    if (parent_inode != 0) {
        rfss_inode_t* parent_inode_ptr = rfss_get_inode(fs, parent_inode);
        if (parent_inode_ptr) {
            static uint8_t block_buffer[RFSS_BLOCK_SIZE];
            for (int i = 0; i < RFSS_DIRECT_BLOCKS && parent_inode_ptr->direct_blocks[i]; i++) {
                if (rfss_read_block(fs, parent_inode_ptr->direct_blocks[i], block_buffer) != 0) {
                    continue;
                }
                uint32_t offset = 0;
                while (offset < RFSS_BLOCK_SIZE) {
                    rfss_dir_entry_t* entry = (rfss_dir_entry_t*)(block_buffer + offset);
                    if (entry->rec_len == 0 || entry->rec_len > RFSS_BLOCK_SIZE - offset ||
                        entry->rec_len < sizeof(rfss_dir_entry_t)) {
                        break;
                    }
                    if (entry->inode == inode_num && entry->name_len == strlen(filename) &&
                        memcmp(entry->name, filename, entry->name_len) == 0) {
                        entry->inode = 0;
                        rfss_write_block(fs, parent_inode_ptr->direct_blocks[i], block_buffer);
                        break;
                    }
                    offset += entry->rec_len;
                }
            }
        }
    }

    kfree(path_copy);
    return 0;
}

int rfss_remove_directory(rfss_fs_t* fs, const char* path) {
    if (!fs || !path || !fs->mounted) {
        return -1;
    }

    uint32_t inode_num = rfss_resolve_path(fs, path);
    if (inode_num == 0) {
        //log(LOG_ERROR, "Directory not found: %s", path);
        return -1;
    }

    rfss_inode_t* inode = rfss_get_inode(fs, inode_num);
    if (!inode || ((inode->mode >> 12) & 0xF) != RFSS_FILE_DIRECTORY) {
        return -1;
    }

    static uint8_t block_buffer[RFSS_BLOCK_SIZE];
    for (int i = 0; i < RFSS_DIRECT_BLOCKS && inode->direct_blocks[i]; i++) {
        if (rfss_read_block(fs, inode->direct_blocks[i], block_buffer) != 0) {
            continue;
        }

        uint32_t offset = 0;
        while (offset < RFSS_BLOCK_SIZE) {
            rfss_dir_entry_t* entry = (rfss_dir_entry_t*)(block_buffer + offset);
            if (entry->rec_len == 0 || entry->rec_len < sizeof(rfss_dir_entry_t)) break;

            if (entry->inode != 0 && entry->name_len > 0 &&
                !(entry->name_len == 1 && entry->name[0] == '.') &&
                !(entry->name_len == 2 && entry->name[0] == '.' && entry->name[1] == '.')) {
                //log(LOG_ERROR, "Directory not empty");
                return -1;
            }

            offset += entry->rec_len;
        }
    }

    for (int i = 0; i < RFSS_DIRECT_BLOCKS && inode->direct_blocks[i]; i++) {
        rfss_free_block(fs, inode->direct_blocks[i]);
    }

    rfss_free_inode(fs, inode_num);

    char* path_copy = kmalloc(strlen(path) + 1);
    if (!path_copy) {
        return -1;
    }
    strcpy(path_copy, path);

    char* dirname = strrchr(path_copy, '/');
    if (dirname) {
        *dirname = '\0';
        dirname++;
    } else {
        dirname = path_copy;
        path_copy = "/";
    }

    uint32_t parent_inode = rfss_resolve_path(fs, path_copy);
    if (parent_inode != 0) {
        rfss_inode_t* parent_inode_ptr = rfss_get_inode(fs, parent_inode);
        if (parent_inode_ptr) {
            static uint8_t block_buffer[RFSS_BLOCK_SIZE];
            for (int i = 0; i < RFSS_DIRECT_BLOCKS && parent_inode_ptr->direct_blocks[i]; i++) {
                if (rfss_read_block(fs, parent_inode_ptr->direct_blocks[i], block_buffer) != 0) {
                    continue;
                }
                uint32_t offset = 0;
                while (offset < RFSS_BLOCK_SIZE) {
                    rfss_dir_entry_t* entry = (rfss_dir_entry_t*)(block_buffer + offset);
                    if (entry->rec_len == 0 || entry->rec_len > RFSS_BLOCK_SIZE - offset ||
                        entry->rec_len < sizeof(rfss_dir_entry_t)) {
                        break;
                    }
                    if (entry->inode == inode_num && entry->name_len == strlen(dirname) &&
                        memcmp(entry->name, dirname, entry->name_len) == 0) {
                        entry->inode = 0;
                        rfss_write_block(fs, parent_inode_ptr->direct_blocks[i], block_buffer);
                        break;
                    }
                    offset += entry->rec_len;
                }
            }
        }
    }

    kfree(path_copy);
    return 0;
}

int rfss_get_stats(rfss_fs_t* fs, uint32_t* total_blocks, uint32_t* free_blocks, uint32_t* total_inodes, uint32_t* free_inodes) {
    if (!fs || !fs->mounted) {
        return -1;
    }

    if (total_blocks) *total_blocks = fs->superblock->total_blocks;
    if (free_blocks) *free_blocks = fs->superblock->free_blocks;
    if (total_inodes) *total_inodes = fs->superblock->inode_count;
    if (free_inodes) *free_inodes = fs->superblock->free_inode_count;

    return 0;
}

int rfss_check_filesystem(rfss_fs_t* fs) {
    if (!fs || !fs->mounted) {
        return -1;
    }

    if (fs->superblock->magic != RFSS_MAGIC) {
        //log(LOG_ERROR, "Invalid magic number");
        return -1;
    }

    if (fs->superblock->version != RFSS_VERSION) {
        //log(LOG_ERROR, "Unsupported version");
        return -1;
    }

    return 0;
}

rfss_fs_t* rfss_get_mounted_fs(void) {
    return mounted_fs;
}

int rfss_create_directory(rfss_fs_t* fs, const char* path) {
    if (!fs || !path || !fs->mounted || strlen(path) == 0 || strlen(path) > 255) {
        //log(LOG_ERROR, "Invalid parameters for create directory");
        return -1;
    }
    
    char* path_copy = kmalloc(strlen(path) + 1);
    if (!path_copy) {
        //log(LOG_ERROR, "Memory allocation failed");
        return -1;
    }
    strcpy(path_copy, path);
    
    int original_absolute = (path[0] == '/') ? 1 : 0;
    char* last_slash = strrchr(path_copy, '/');
    char* dirname;
    char* parent_path_str;
    if (last_slash) {
        *last_slash = '\0';
        dirname = last_slash + 1;
        parent_path_str = path_copy;
    } else {
        dirname = path_copy;
        parent_path_str = NULL;
    }
    
    if (strlen(dirname) == 0 || strlen(dirname) >= RFSS_MAX_FILENAME) {
        //log(LOG_ERROR, "Invalid directory name");
        kfree(path_copy);
        return -1;
    }
    
    uint32_t parent_inode;
    if (!parent_path_str || strlen(parent_path_str) == 0) {
        if (original_absolute) {
            parent_inode = fs->superblock->root_inode;
        } else {
            parent_inode = fs->current_dir_inode;
        }
    } else {
        parent_inode = rfss_resolve_path(fs, parent_path_str);
        if (parent_inode == 0) {
            //log(LOG_ERROR, "Parent directory not found");
            kfree(path_copy);
            return -1;
        }
    }
    
    if (rfss_find_file_in_directory(fs, parent_inode, dirname) != 0) {
        //log(LOG_ERROR, "Directory already exists: %s", dirname);
        kfree(path_copy);
        return -1;
    }
    
    uint32_t new_inode_num = rfss_allocate_inode(fs);
    if (new_inode_num == 0) {
        //log(LOG_ERROR, "Failed to allocate inode for directory");
        kfree(path_copy);
        return -1;
    }
    
    uint32_t dir_block = rfss_allocate_block(fs);
    if (dir_block == 0) {
        //log(LOG_ERROR, "Failed to allocate block for directory");
        rfss_free_inode(fs, new_inode_num);
        kfree(path_copy);
        return -1;
    }
    
    //log(LOG_DEBUG, "Creating directory '%s': inode=%d, block=%d", dirname, new_inode_num, dir_block);
    
    rfss_inode_t new_inode;
    memset(&new_inode, 0, sizeof(rfss_inode_t));
    new_inode.mode = 0755 | (RFSS_FILE_DIRECTORY << 12);
    new_inode.uid = 0;
    new_inode.gid = 0;
    new_inode.size = RFSS_BLOCK_SIZE;
    new_inode.links_count = 2;
    new_inode.blocks_count = 1;
    new_inode.direct_blocks[0] = dir_block;
    
    static uint8_t block_buffer[RFSS_BLOCK_SIZE];
    memset(block_buffer, 0, RFSS_BLOCK_SIZE);
    
    rfss_dir_entry_t* dot_entry = (rfss_dir_entry_t*)block_buffer;
    dot_entry->inode = new_inode_num;
    dot_entry->rec_len = 12;
    dot_entry->name_len = 1;
    dot_entry->file_type = RFSS_FILE_DIRECTORY;
    dot_entry->name[0] = '.';
    memset(&dot_entry->name[1], 0, RFSS_MAX_FILENAME - 1);
    
    rfss_dir_entry_t* dotdot_entry = (rfss_dir_entry_t*)(block_buffer + 12);
    dotdot_entry->inode = parent_inode;
    dotdot_entry->rec_len = RFSS_BLOCK_SIZE - 12;
    dotdot_entry->name_len = 2;
    dotdot_entry->file_type = RFSS_FILE_DIRECTORY;
    dotdot_entry->name[0] = '.';
    dotdot_entry->name[1] = '.';
    memset(&dotdot_entry->name[2], 0, RFSS_MAX_FILENAME - 2);
    
    if (rfss_write_block(fs, dir_block, block_buffer) != 0) {
        //log(LOG_ERROR, "Failed to write directory block");
        rfss_free_block(fs, dir_block);
        rfss_free_inode(fs, new_inode_num);
        kfree(path_copy);
        return -1;
    }
    
    if (rfss_write_inode(fs, new_inode_num, &new_inode) != 0) {
        //log(LOG_ERROR, "Failed to write directory inode");
        rfss_free_block(fs, dir_block);
        rfss_free_inode(fs, new_inode_num);
        kfree(path_copy);
        return -1;
    }
    
    if (rfss_add_directory_entry(fs, parent_inode, dirname, new_inode_num, RFSS_FILE_DIRECTORY) != 0) {
        //log(LOG_ERROR, "Failed to add directory entry to parent");
        rfss_free_block(fs, dir_block);
        rfss_free_inode(fs, new_inode_num);
        kfree(path_copy);
        return -1;
    }
    
    //log(LOG_DEBUG, "Successfully created directory: %s", path);
    kfree(path_copy);
    return 0;
}

int rfss_open_file(rfss_fs_t* fs, const char* path, int flags, rfss_file_t* file) {
    if (!fs || !path || !file || !fs->mounted) {
        //log(LOG_ERROR, "Invalid parameters for open file");
        return -1;
    }
    
    uint32_t inode_num = rfss_resolve_path(fs, path);
    if (inode_num == 0) {
        //log(LOG_ERROR, "File not found: %s", path);
        return -1;
    }
    
    rfss_inode_t* inode = rfss_get_inode(fs, inode_num);
    if (!inode) {
        //log(LOG_ERROR, "Failed to get inode %d", inode_num);
        return -1;
    }
    
    file->inode = kmalloc(sizeof(rfss_inode_t));
    if (!file->inode) {
        //log(LOG_ERROR, "Memory allocation failed for file inode");
        return -1;
    }
    
    memcpy(file->inode, inode, sizeof(rfss_inode_t));
    file->inode_num = inode_num;
    file->position = 0;
    file->flags = flags;
    file->fs = fs;

    // Truncate file if opened for writing
    if (flags & 1) {
        for (int i = 0; i < RFSS_DIRECT_BLOCKS && file->inode->direct_blocks[i]; i++) {
            rfss_free_block(fs, file->inode->direct_blocks[i]);
            file->inode->direct_blocks[i] = 0;
        }
        file->inode->size = 0;
        file->inode->blocks_count = 0;
        rfss_write_inode(fs, inode_num, file->inode);
    }
    
    //log(LOG_DEBUG, "Opened file: %s (inode %d)", path, inode_num);
    return 0;
}

int rfss_close_file(rfss_file_t* file) {
    if (!file || !file->inode) {
        return -1;
    }
    
    kfree(file->inode);
    memset(file, 0, sizeof(rfss_file_t));
    return 0;
}

int rfss_read_file(rfss_file_t* file, void* buffer, size_t size) {
    if (!file || !file->inode || !buffer || !file->fs || size == 0) {
        return -1;
    }
    
    if (file->position >= file->inode->size) {
        return 0;
    }
    
    if (file->position + size > file->inode->size) {
        size = file->inode->size - file->position;
    }
    
    size_t bytes_read = 0;
    static uint8_t block_buffer[RFSS_BLOCK_SIZE];
    
    while (bytes_read < size) {
        uint32_t block_index = (file->position + bytes_read) / RFSS_BLOCK_SIZE;
        uint32_t block_offset = (file->position + bytes_read) % RFSS_BLOCK_SIZE;
        uint32_t block_num = 0;
        
        if (block_index < RFSS_DIRECT_BLOCKS) {
            block_num = file->inode->direct_blocks[block_index];
        }
        
        if (block_num == 0) {
            break;
        }
        
        if (rfss_read_block(file->fs, block_num, block_buffer) != 0) {
            break;
        }
        
        size_t copy_size = RFSS_BLOCK_SIZE - block_offset;
        if (copy_size > size - bytes_read) {
            copy_size = size - bytes_read;
        }
        
        memcpy((uint8_t*)buffer + bytes_read, block_buffer + block_offset, copy_size);
        bytes_read += copy_size;
    }
    
    file->position += bytes_read;
    return bytes_read;
}

int rfss_write_file(rfss_file_t* file, const void* buffer, size_t size) {
    if (!file || !file->inode || !buffer || !file->fs || size == 0) {
        return -1;
    }
    
    size_t bytes_written = 0;
    static uint8_t block_buffer[RFSS_BLOCK_SIZE];
    
    while (bytes_written < size) {
        uint32_t block_index = (file->position + bytes_written) / RFSS_BLOCK_SIZE;
        uint32_t block_offset = (file->position + bytes_written) % RFSS_BLOCK_SIZE;
        uint32_t block_num = 0;
        
        if (block_index < RFSS_DIRECT_BLOCKS) {
            if (file->inode->direct_blocks[block_index] == 0) {
                file->inode->direct_blocks[block_index] = rfss_allocate_block(file->fs);
                if (file->inode->direct_blocks[block_index] == 0) {
                    break;
                }
                file->inode->blocks_count++;
                memset(block_buffer, 0, RFSS_BLOCK_SIZE);
            } else {
                if (rfss_read_block(file->fs, file->inode->direct_blocks[block_index], block_buffer) != 0) {
                    break;
                }
            }
            block_num = file->inode->direct_blocks[block_index];
        }
        
        if (block_num == 0) {
            break;
        }
        
        size_t copy_size = RFSS_BLOCK_SIZE - block_offset;
        if (copy_size > size - bytes_written) {
            copy_size = size - bytes_written;
        }
        
        memcpy(block_buffer + block_offset, (uint8_t*)buffer + bytes_written, copy_size);
        
        if (rfss_write_block(file->fs, block_num, block_buffer) != 0) {
            break;
        }
        
        bytes_written += copy_size;
    }
    
    file->position += bytes_written;
    if (file->position > file->inode->size) {
        file->inode->size = file->position;
    }
    
    rfss_write_inode(file->fs, file->inode_num, file->inode);
    return bytes_written;
}

int rfss_change_directory(rfss_fs_t* fs, const char* path) {
    if (!fs || !path || !fs->mounted) {
        return -1;
    }

    uint32_t inode_num = rfss_resolve_path(fs, path);
    if (inode_num == 0) {
        return -1;
    }

    rfss_inode_t* inode = rfss_get_inode(fs, inode_num);
    if (!inode || ((inode->mode >> 12) & 0xF) != RFSS_FILE_DIRECTORY) {
        return -1;
    }

    fs->current_dir_inode = inode_num;

    return 0;
}

int rfss_list_directory(rfss_fs_t* fs, const char* path, rfss_dir_entry_t** entries, int* count) {
    if (!fs || !entries || !count || !fs->mounted) {
        return -1;
    }
    
    uint32_t inode_num;
    if (path && strlen(path) > 0) {
        inode_num = rfss_resolve_path(fs, path);
    } else {
        inode_num = fs->current_dir_inode;
    }
    
    if (inode_num == 0) {
        return -1;
    }
    
    rfss_inode_t* inode = rfss_get_inode(fs, inode_num);
    if (!inode || ((inode->mode >> 12) & 0xF) != RFSS_FILE_DIRECTORY) {
        return -1;
    }
    
    *count = 0;
    *entries = NULL;
    
    static uint8_t block_buffer[RFSS_BLOCK_SIZE];
    for (int i = 0; i < RFSS_DIRECT_BLOCKS && inode->direct_blocks[i]; i++) {
        if (rfss_read_block(fs, inode->direct_blocks[i], block_buffer) != 0) {
            continue;
        }
        
        uint32_t offset = 0;
        while (offset < RFSS_BLOCK_SIZE) {
            rfss_dir_entry_t* entry = (rfss_dir_entry_t*)(block_buffer + offset);
            if (entry->rec_len == 0 || entry->rec_len > RFSS_BLOCK_SIZE - offset ||
                entry->rec_len < sizeof(rfss_dir_entry_t)) {
                break;
            }
            
            if (entry->inode != 0 && entry->name_len > 0 && entry->name_len < RFSS_MAX_FILENAME) {
                (*count)++;
            }
            
            offset += entry->rec_len;
        }
    }
    
    if (*count == 0) {
        return 0;
    }
    
    int rfss_enable_journaling(rfss_fs_t* fs) {
        if (!fs || !fs->mounted) {
            return -1;
        }
    
        if (fs->superblock->journal_size == 0) {
            //log(LOG_ERROR, "Journal not configured in superblock");
            return -1;
        }
    
        if (!fs->journaling_enabled) {
            fs->journaling_enabled = 1;
            rfss_journal_init(fs);
            log(LOG_OK, "Journaling enabled");
        }
    
        return 0;
    }
    
    int rfss_disable_journaling(rfss_fs_t* fs) {
        if (!fs || !fs->mounted) {
            return -1;
        }
    
        if (fs->journaling_enabled) {
            fs->journaling_enabled = 0;
            log(LOG_OK, "Journaling disabled");
        }
    
        return 0;
    }
    
    *entries = kmalloc(*count * sizeof(rfss_dir_entry_t));
    if (!*entries) {
        return -1;
    }
    
    int entry_index = 0;
    for (int i = 0; i < RFSS_DIRECT_BLOCKS && inode->direct_blocks[i] && entry_index < *count; i++) {
        if (rfss_read_block(fs, inode->direct_blocks[i], block_buffer) != 0) {
            continue;
        }
        
        uint32_t offset = 0;
        while (offset < RFSS_BLOCK_SIZE && entry_index < *count) {
            rfss_dir_entry_t* entry = (rfss_dir_entry_t*)(block_buffer + offset);
            if (entry->rec_len == 0 || entry->rec_len > RFSS_BLOCK_SIZE - offset ||
                entry->rec_len < sizeof(rfss_dir_entry_t)) {
                break;
            }
            
            if (entry->inode != 0 && entry->name_len > 0 && entry->name_len < RFSS_MAX_FILENAME) {
                memcpy(&(*entries)[entry_index], entry, sizeof(rfss_dir_entry_t));
                entry_index++;
            }
            
            offset += entry->rec_len;
        }
    }
    
    return 0;
}