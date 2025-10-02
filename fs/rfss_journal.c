#include "rfss.h"
#include "../drivers/ata.h"
#include "../mm/memory.h"
#include "../kernel/logger.h"
#include <string.h>

static uint32_t current_transaction_id = 1;

typedef struct {
    uint32_t transaction_id;
    uint32_t block_count;
    uint32_t blocks[64];
    uint8_t* backup_data[64];
} rfss_transaction_t;

static rfss_transaction_t* current_transaction = NULL;

int rfss_journal_init(rfss_fs_t* fs) {
    if (!fs || !fs->mounted) {
        return -1;
    }
    
    uint8_t journal_buffer[RFSS_BLOCK_SIZE];
    memset(journal_buffer, 0, RFSS_BLOCK_SIZE);
    
    for (uint32_t i = 0; i < fs->superblock->journal_size; i++) {
        if (rfss_write_block(fs, fs->superblock->journal_block + i, journal_buffer) != 0) {
            return -1;
        }
    }
    
    log(LOG_OK, "Journal initialized");
    return 0;
}

int rfss_journal_start_transaction(rfss_fs_t* fs) {
    if (!fs || current_transaction) {
        return -1;
    }
    
    current_transaction = kmalloc(sizeof(rfss_transaction_t));
    if (!current_transaction) {
        return -1;
    }
    
    current_transaction->transaction_id = current_transaction_id++;
    current_transaction->block_count = 0;
    
    for (int i = 0; i < 64; i++) {
        current_transaction->backup_data[i] = NULL;
    }
    
    return 0;
}

int rfss_journal_log_block(rfss_fs_t* fs, uint32_t block_num, const void* old_data) {
    if (!fs || !current_transaction || current_transaction->block_count >= 64) {
        return -1;
    }
    
    uint32_t index = current_transaction->block_count;
    current_transaction->blocks[index] = block_num;
    current_transaction->backup_data[index] = kmalloc(RFSS_BLOCK_SIZE);
    
    if (!current_transaction->backup_data[index]) {
        return -1;
    }
    
    memcpy(current_transaction->backup_data[index], old_data, RFSS_BLOCK_SIZE);
    current_transaction->block_count++;
    
    return 0;
}

int rfss_journal_commit_transaction(rfss_fs_t* fs) {
    if (!fs || !current_transaction) {
        return -1;
    }
    
    rfss_journal_header_t header;
    header.transaction_id = current_transaction->transaction_id;
    header.block_count = current_transaction->block_count;
    header.timestamp = 0;
    header.checksum = rfss_calculate_checksum(&header, sizeof(rfss_journal_header_t) - sizeof(uint32_t));
    
    uint8_t journal_buffer[RFSS_BLOCK_SIZE];
    memset(journal_buffer, 0, RFSS_BLOCK_SIZE);
    memcpy(journal_buffer, &header, sizeof(rfss_journal_header_t));
    
    uint32_t journal_offset = sizeof(rfss_journal_header_t);
    for (uint32_t i = 0; i < current_transaction->block_count; i++) {
        rfss_journal_block_t journal_block;
        journal_block.block_num = current_transaction->blocks[i];
        journal_block.old_checksum = rfss_calculate_checksum(current_transaction->backup_data[i], RFSS_BLOCK_SIZE);
        
        uint8_t new_data[RFSS_BLOCK_SIZE];
        if (rfss_read_block(fs, current_transaction->blocks[i], new_data) == 0) {
            journal_block.new_checksum = rfss_calculate_checksum(new_data, RFSS_BLOCK_SIZE);
        } else {
            journal_block.new_checksum = 0;
        }
        
        memcpy(journal_block.data, current_transaction->backup_data[i], RFSS_BLOCK_SIZE);
        
        if (journal_offset + sizeof(rfss_journal_block_t) > RFSS_BLOCK_SIZE) {
            if (rfss_write_block(fs, fs->superblock->journal_block, journal_buffer) != 0) {
                return -1;
            }
            memset(journal_buffer, 0, RFSS_BLOCK_SIZE);
            journal_offset = 0;
        }
        
        memcpy(journal_buffer + journal_offset, &journal_block, sizeof(rfss_journal_block_t));
        journal_offset += sizeof(rfss_journal_block_t);
    }
    
    if (journal_offset > 0) {
        if (rfss_write_block(fs, fs->superblock->journal_block, journal_buffer) != 0) {
            return -1;
        }
    }
    
    for (uint32_t i = 0; i < current_transaction->block_count; i++) {
        if (current_transaction->backup_data[i]) {
            kfree(current_transaction->backup_data[i]);
        }
    }
    
    kfree(current_transaction);
    current_transaction = NULL;
    
    return 0;
}

int rfss_journal_abort_transaction(rfss_fs_t* fs) {
    if (!fs || !current_transaction) {
        return -1;
    }
    
    for (uint32_t i = 0; i < current_transaction->block_count; i++) {
        if (current_transaction->backup_data[i]) {
            rfss_write_block(fs, current_transaction->blocks[i], current_transaction->backup_data[i]);
            kfree(current_transaction->backup_data[i]);
        }
    }
    
    kfree(current_transaction);
    current_transaction = NULL;
    
    return 0;
}

int rfss_journal_replay(rfss_fs_t* fs) {
    if (!fs || !fs->mounted) {
        return -1;
    }
    
    uint8_t journal_buffer[RFSS_BLOCK_SIZE];
    if (rfss_read_block(fs, fs->superblock->journal_block, journal_buffer) != 0) {
        return -1;
    }
    
    rfss_journal_header_t* header = (rfss_journal_header_t*)journal_buffer;
    if (header->transaction_id == 0) {
        return 0;
    }
    
    uint32_t calculated_checksum = rfss_calculate_checksum(header, sizeof(rfss_journal_header_t) - sizeof(uint32_t));
    if (calculated_checksum != header->checksum) {
        log(LOG_ERROR, "Journal header checksum mismatch");
        return -1;
    }
    
    log(LOG_LOG, "Replaying journal transaction %d with %d blocks", header->transaction_id, header->block_count);
    
    uint32_t journal_offset = sizeof(rfss_journal_header_t);
    for (uint32_t i = 0; i < header->block_count; i++) {
        if (journal_offset + sizeof(rfss_journal_block_t) > RFSS_BLOCK_SIZE) {
            if (rfss_read_block(fs, fs->superblock->journal_block + 1, journal_buffer) != 0) {
                return -1;
            }
            journal_offset = 0;
        }
        
        rfss_journal_block_t* journal_block = (rfss_journal_block_t*)(journal_buffer + journal_offset);
        
        uint8_t current_data[RFSS_BLOCK_SIZE];
        if (rfss_read_block(fs, journal_block->block_num, current_data) == 0) {
            uint32_t current_checksum = rfss_calculate_checksum(current_data, RFSS_BLOCK_SIZE);
            
            if (current_checksum == journal_block->new_checksum) {
                log(LOG_LOG, "Block %d already updated, skipping", journal_block->block_num);
            } else if (current_checksum == journal_block->old_checksum) {
                log(LOG_LOG, "Restoring block %d from journal", journal_block->block_num);
                rfss_write_block(fs, journal_block->block_num, journal_block->data);
            } else {
                log(LOG_WARNING, "Block %d checksum mismatch, potential corruption", journal_block->block_num);
            }
        }
        
        journal_offset += sizeof(rfss_journal_block_t);
    }
    
    memset(journal_buffer, 0, RFSS_BLOCK_SIZE);
    rfss_write_block(fs, fs->superblock->journal_block, journal_buffer);
    
    log(LOG_OK, "Journal replay completed");
    return 0;
}

int rfss_journal_clear(rfss_fs_t* fs) {
    if (!fs || !fs->mounted) {
        return -1;
    }
    
    uint8_t journal_buffer[RFSS_BLOCK_SIZE];
    memset(journal_buffer, 0, RFSS_BLOCK_SIZE);
    
    for (uint32_t i = 0; i < fs->superblock->journal_size; i++) {
        if (rfss_write_block(fs, fs->superblock->journal_block + i, journal_buffer) != 0) {
            return -1;
        }
    }
    
    return 0;
}

int rfss_journal_check_consistency(rfss_fs_t* fs) {
    if (!fs || !fs->mounted) {
        return -1;
    }
    
    uint8_t journal_buffer[RFSS_BLOCK_SIZE];
    if (rfss_read_block(fs, fs->superblock->journal_block, journal_buffer) != 0) {
        return -1;
    }
    
    rfss_journal_header_t* header = (rfss_journal_header_t*)journal_buffer;
    if (header->transaction_id == 0) {
        return 0;
    }
    
    uint32_t calculated_checksum = rfss_calculate_checksum(header, sizeof(rfss_journal_header_t) - sizeof(uint32_t));
    if (calculated_checksum != header->checksum) {
        log(LOG_ERROR, "Journal inconsistency detected");
        return -1;
    }
    
    return 0;
}

int rfss_safe_write_block(rfss_fs_t* fs, uint32_t block_num, const void* data) {
    if (!fs || !data) {
        return -1;
    }
    
    uint8_t old_data[RFSS_BLOCK_SIZE];
    if (rfss_read_block(fs, block_num, old_data) != 0) {
        memset(old_data, 0, RFSS_BLOCK_SIZE);
    }
    
    if (current_transaction) {
        if (rfss_journal_log_block(fs, block_num, old_data) != 0) {
            return -1;
        }
    }
    
    return rfss_write_block(fs, block_num, data);
}

int rfss_safe_write_inode(rfss_fs_t* fs, uint32_t inode_num, rfss_inode_t* inode) {
    if (!fs || !inode || inode_num == 0 || inode_num > fs->superblock->inode_count) {
        return -1;
    }
    
    uint32_t inodes_per_block = RFSS_BLOCK_SIZE / sizeof(rfss_inode_t);
    uint32_t block = (inode_num - 1) / inodes_per_block;
    uint32_t index = (inode_num - 1) % inodes_per_block;
    
    uint8_t old_block_data[RFSS_BLOCK_SIZE];
    if (rfss_read_block(fs, fs->superblock->inode_table_block + block, old_block_data) != 0) {
        return -1;
    }
    
    if (current_transaction) {
        if (rfss_journal_log_block(fs, fs->superblock->inode_table_block + block, old_block_data) != 0) {
            return -1;
        }
    }
    
    rfss_inode_t* inodes = (rfss_inode_t*)old_block_data;
    memcpy(&inodes[index], inode, sizeof(rfss_inode_t));
    
    return rfss_write_block(fs, fs->superblock->inode_table_block + block, old_block_data);
}