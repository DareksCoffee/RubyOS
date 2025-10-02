#include "memory.h"
#include <logger.h>
#include <stdio.h>
#include <../drivers/multi_boot.h>

memory_map_t memory_map = {0};

void detect_memory(multiboot_info_t* mbd) {
    if (!(mbd->flags & 0x40)) { // Check if memory map is available
        log(LOG_ERROR, "No memory map provided by GRUB!");
        return;
    }

    uint32_t entries = 0;
    uint32_t addr = mbd->mmap_addr;
    
    while (addr < mbd->mmap_addr + mbd->mmap_length && entries < MEMORY_MAX_ENTRIES) {
        multiboot_memory_map_t* mb_entry = (multiboot_memory_map_t*)addr;
        
        memory_map.map[entries].base = mb_entry->addr;
        memory_map.map[entries].length = mb_entry->len;
        
        // Convert multiboot type to our type
        switch (mb_entry->type) {
            case 1: // Available RAM
                memory_map.map[entries].type = MEMORY_TYPE_FREE;
                break;
            case 3: // ACPI reclaimable
                memory_map.map[entries].type = MEMORY_TYPE_ACPI_RECLAIMABLE;
                break;
            case 4: // Reserved for hibernation
                memory_map.map[entries].type = MEMORY_TYPE_ACPI_NVS;
                break;
            case 5: // Defective
                memory_map.map[entries].type = MEMORY_TYPE_BAD;
                break;
            default: // Everything else is reserved
                memory_map.map[entries].type = MEMORY_TYPE_RESERVED;
                break;
        }
        
        entries++;
        addr += mb_entry->size + sizeof(mb_entry->size);
    }

    memory_map.entries = entries;
    log(LOG_OK, "Memory map detected with %d entries", entries);
}

void print_memory_map(void) {
    printf("\nMemory Map:\n");
    printf("Base Address          | Length               | Type\n");
    printf("-------------------------------------------|----------\n");

    for (uint32_t i = 0; i < memory_map.entries; i++) {
        memory_map_entry_t* entry = &memory_map.map[i];
        printf("0x%016llx | 0x%016llx | %d\n",
               entry->base,
               entry->length,
               entry->type);
    }
}

uint64_t get_total_memory(void) {
    uint64_t total = 0;
    for (uint32_t i = 0; i < memory_map.entries; i++) {
        if (memory_map.map[i].type == MEMORY_TYPE_FREE) {
            total += memory_map.map[i].length;
        }
    }
    return total;
}

static memory_stats_t mem_stats = {0};

void init_memory_manager(multiboot_info_t* mbd) {
    detect_memory(mbd);
    
    mem_stats.total_memory = get_total_memory();
    mem_stats.free_memory = mem_stats.total_memory;
    mem_stats.used_memory = 0;
    
    mem_stats.total_pages = mem_stats.total_memory / PAGE_SIZE;
    mem_stats.free_pages = mem_stats.total_pages;
    mem_stats.used_pages = 0;
}

memory_stats_t get_memory_stats(void) {
    return mem_stats;
}

static uint8_t heap[1024 * 1024];
static uint32_t heap_offset = 0;

void* kmalloc(size_t size) {
    if (heap_offset + size >= sizeof(heap)) {
        return NULL;
    }
    
    void* ptr = &heap[heap_offset];
    heap_offset += size;
    heap_offset = (heap_offset + 3) & ~3;
    
    mem_stats.used_memory += size;
    mem_stats.free_memory -= size;
    
    return ptr;
}

void kfree(void* ptr) {
    (void)ptr;
}