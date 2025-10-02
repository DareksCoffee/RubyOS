#include "rfss.h"
#include "../drivers/ata.h"
#include "../mm/memory.h"
#include "../kernel/logger.h"
#include <string.h>

static rfss_inode_t* rfss_get_inode(rfss_fs_t* fs, uint32_t inode_num) {
    if (!fs || inode_num == 0 || inode_num > fs->superblock->inode_count) {
        return NULL;
    }
    
    uint32_t inodes_per_block = RFSS_BLOCK_SIZE / sizeof(rfss_inode_t);
    uint32_t block = (inode_num - 1) / inodes_per_block;
    uint32_t index = (inode_num - 1) % inodes_per_block;
    
    static rfss_inode_t inodes[RFSS_BLOCK_SIZE / sizeof(rfss_inode_t)];
    if (rfss_read_block(fs, fs->superblock->inode_table_block + block, inodes) != 0) {
        return NULL;
    }
    
    return &inodes[index];
}

static int rfss_write_inode(rfss_fs_t* fs, uint32_t inode_num, rfss_inode_t* inode) {
    if (!fs || !inode || inode_num == 0 || inode_num > fs->superblock->inode_count) {
        return -1;
    }
    
    uint32_t inodes_per_block = RFSS_BLOCK_SIZE / sizeof(rfss_inode_t);
    uint32_t block = (inode_num - 1) / inodes_per_block;
    uint32_t index = (inode_num - 1) % inodes_per_block;
    
    rfss_inode_t inodes[inodes_per_block];
    if (rfss_read_block(fs, fs->superblock->inode_table_block + block, inodes) != 0) {
        return -1;
    }
    
    memcpy(&inodes[index], inode, sizeof(rfss_inode_t));
    return rfss_write_block(fs, fs->superblock->inode_table_block + block, inodes);
}

static uint32_t rfss_find_file_in_directory(rfss_fs_t* fs, uint32_t dir_inode, const char* name) {
    rfss_inode_t* inode = rfss_get_inode(fs, dir_inode);
    if (!inode || ((inode->mode >> 12) & 0xF) != RFSS_FILE_DIRECTORY) {
        return 0;
    }
    
    uint8_t block_buffer[RFSS_BLOCK_SIZE];
    for (int i = 0; i < RFSS_DIRECT_BLOCKS && inode->direct_blocks[i]; i++) {
        if (rfss_read_block(fs, inode->direct_blocks[i], block_buffer) != 0) {
            continue;
        }
        
        uint32_t offset = 0;
        while (offset < RFSS_BLOCK_SIZE) {
            rfss_dir_entry_t* entry = (rfss_dir_entry_t*)(block_buffer + offset);
            if (entry->rec_len == 0) break;
            
            if (entry->inode != 0 && entry->name_len == strlen(name) && 
                strncmp(entry->name, name, entry->name_len) == 0) {
                return entry->inode;
            }
            
            offset += entry->rec_len;
        }
    }
    
    return 0;
}

