#include "gdt.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct gdt_entry gdt[5];
struct gdt_ptr gdtp;

extern void gdt_flush(uint32_t);

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access = access;
}

void init_gdt() {
    gdtp.limit = (sizeof(struct gdt_entry) * 5) - 1;
    gdtp.base = (uint32_t)&gdt;

    // Null segment
    gdt_set_gate(0, 0, 0, 0, 0);
    // Kernel code segment - ring 0
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    // Kernel data segment - ring 0
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    // User code segment - ring 3 (not used yet)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    // User data segment - ring 3 (not used yet)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    gdt_flush((uint32_t)&gdtp);
}

void prnt_gdtinfo(void) {
    printf("........ GDT Pointer:\n");
    printf("............ base  = 0x%08X\n", gdtp.base);
    printf("............ limit = 0x%04X\n", gdtp.limit);

    for (int i = 0; i < 5; i++) {
        printf("........ Entry %d:\n", i);
        printf("............ base   = 0x%02X%02X%04X\n",
               gdt[i].base_high,
               gdt[i].base_middle,
               gdt[i].base_low);
        printf("............ limit  = 0x%01X%04X\n",
               (gdt[i].granularity >> 4) & 0x0F,
               gdt[i].limit_low);
        printf("............ access = 0x%02X\n", gdt[i].access);
        printf("............ gran   = 0x%02X\n", gdt[i].granularity & 0xF0);
    }
}
