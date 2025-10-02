#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <../drivers/multi_boot.h>

#define MEMORY_MAX_ENTRIES 32

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) memory_map_entry_t;

typedef struct {
    uint32_t entries;
    memory_map_entry_t map[MEMORY_MAX_ENTRIES];
} memory_map_t;

extern memory_map_t memory_map;

void detect_memory(multiboot_info_t* mbd);
void print_memory_map(void);
uint64_t get_total_memory(void);

#define MEMORY_TYPE_FREE 1
#define MEMORY_TYPE_RESERVED 2
#define MEMORY_TYPE_ACPI_RECLAIMABLE 3
#define MEMORY_TYPE_ACPI_NVS 4
#define MEMORY_TYPE_BAD 5

#define PAGE_SIZE 4096

typedef struct {
    uint64_t total_memory;
    uint64_t used_memory;
    uint64_t free_memory;
    uint32_t total_pages;
    uint32_t used_pages;
    uint32_t free_pages;
} memory_stats_t;

void init_memory_manager(multiboot_info_t* mbd);
memory_stats_t get_memory_stats(void);

void* kmalloc(size_t size);
void kfree(void* ptr);

#endif