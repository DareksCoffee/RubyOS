#ifndef RFSS_H
#define RFSS_H

#include <stdint.h>
#include <stddef.h>

#define RFSS_MAGIC 0x52465353
#define RFSS_VERSION 1
#define RFSS_BLOCK_SIZE 4096
#define RFSS_MAX_FILENAME 255
#define RFSS_DIRECT_BLOCKS 12
#define RFSS_INDIRECT_BLOCKS 1
#define RFSS_DOUBLE_INDIRECT_BLOCKS 1
#define RFSS_TRIPLE_INDIRECT_BLOCKS 1
#define RFSS_MAX_EXTENTS 8

typedef enum {
    RFSS_FILE_REGULAR = 1,
    RFSS_FILE_DIRECTORY = 2,
    RFSS_FILE_SYMLINK = 3,
    RFSS_FILE_DEVICE = 4
} rfss_file_type_t;

typedef struct {
    uint32_t start_block;
    uint32_t length;
} rfss_extent_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t free_blocks;
    uint32_t inode_table_block;
    uint32_t inode_count;
    uint32_t free_inode_count;
    uint32_t root_inode;
    uint32_t bitmap_block;
    uint32_t journal_block;
    uint32_t journal_size;
    uint64_t created_time;
    uint64_t modified_time;
    uint32_t mount_count;
    uint32_t max_mount_count;
    uint32_t state;
    uint32_t errors;
    uint8_t uuid[16];
    char label[16];
    uint8_t reserved[928];
} __attribute__((packed)) rfss_superblock_t;

typedef struct {
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    uint64_t atime;
    uint64_t ctime;
    uint64_t mtime;
    uint32_t links_count;
    uint32_t blocks_count;
    uint32_t flags;
    uint32_t direct_blocks[RFSS_DIRECT_BLOCKS];
    uint32_t indirect_block;
    uint32_t double_indirect_block;
    uint32_t triple_indirect_block;
    rfss_extent_t extents[RFSS_MAX_EXTENTS];
    uint32_t extent_count;
    uint8_t reserved[64];
} __attribute__((packed)) rfss_inode_t;

typedef struct {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[RFSS_MAX_FILENAME];
} __attribute__((packed)) rfss_dir_entry_t;

typedef struct {
    uint32_t transaction_id;
    uint32_t block_count;
    uint64_t timestamp;
    uint32_t checksum;
    uint8_t data[];
} __attribute__((packed)) rfss_journal_header_t;

typedef struct {
    uint32_t block_num;
    uint32_t old_checksum;
    uint32_t new_checksum;
    uint8_t data[RFSS_BLOCK_SIZE];
} __attribute__((packed)) rfss_journal_block_t;

typedef struct {
    rfss_superblock_t* superblock;
    uint8_t* block_bitmap;
    uint8_t* inode_bitmap;
    rfss_inode_t* inode_table;
    uint32_t current_dir_inode;
    char current_path[1024];
    uint32_t device_id;
    int mounted;
    int dirty;
    int journaling_enabled;
} rfss_fs_t;

typedef struct {
    rfss_inode_t* inode;
    uint32_t inode_num;
    uint64_t position;
    int flags;
    rfss_fs_t* fs;
} rfss_file_t;

int rfss_format(uint32_t device_id, const char* label);
int rfss_mount(uint32_t device_id, rfss_fs_t* fs);
int rfss_unmount(rfss_fs_t* fs);
int rfss_create_file(rfss_fs_t* fs, const char* path, uint32_t mode);
int rfss_delete_file(rfss_fs_t* fs, const char* path);
int rfss_open_file(rfss_fs_t* fs, const char* path, int flags, rfss_file_t* file);
int rfss_close_file(rfss_file_t* file);
int rfss_read_file(rfss_file_t* file, void* buffer, size_t size);
int rfss_write_file(rfss_file_t* file, const void* buffer, size_t size);
int rfss_create_directory(rfss_fs_t* fs, const char* path);
int rfss_remove_directory(rfss_fs_t* fs, const char* path);
int rfss_list_directory(rfss_fs_t* fs, const char* path, rfss_dir_entry_t** entries, int* count);
int rfss_change_directory(rfss_fs_t* fs, const char* path);
int rfss_get_stats(rfss_fs_t* fs, uint32_t* total_blocks, uint32_t* free_blocks, uint32_t* total_inodes, uint32_t* free_inodes);
int rfss_check_filesystem(rfss_fs_t* fs);
rfss_fs_t* rfss_get_mounted_fs(void);

uint32_t rfss_allocate_block(rfss_fs_t* fs);
void rfss_free_block(rfss_fs_t* fs, uint32_t block);
uint32_t rfss_allocate_inode(rfss_fs_t* fs);
void rfss_free_inode(rfss_fs_t* fs, uint32_t inode);
int rfss_read_block(rfss_fs_t* fs, uint32_t block, void* buffer);
int rfss_write_block(rfss_fs_t* fs, uint32_t block, const void* buffer);
uint32_t rfss_calculate_checksum(const void* data, size_t size);

int rfss_journal_init(rfss_fs_t* fs);
int rfss_journal_start_transaction(rfss_fs_t* fs);
int rfss_journal_log_block(rfss_fs_t* fs, uint32_t block_num, const void* old_data);
int rfss_journal_commit_transaction(rfss_fs_t* fs);
int rfss_journal_abort_transaction(rfss_fs_t* fs);
int rfss_journal_replay(rfss_fs_t* fs);
int rfss_journal_clear(rfss_fs_t* fs);
int rfss_journal_check_consistency(rfss_fs_t* fs);
int rfss_safe_write_block(rfss_fs_t* fs, uint32_t block_num, const void* data);
int rfss_safe_write_inode(rfss_fs_t* fs, uint32_t inode_num, rfss_inode_t* inode);

int rfss_enable_journaling(rfss_fs_t* fs);
int rfss_disable_journaling(rfss_fs_t* fs);

#endif