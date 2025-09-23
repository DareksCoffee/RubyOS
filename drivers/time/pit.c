#include "pit.h"
#include <../cpu/ports.h>

static volatile uint64_t ticks = 0;

void pit_handler() {
    ticks++;
}

void pit_init(uint32_t frequency) {
    uint32_t divisor = 1193182 / frequency;

    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    ticks = 0;
    // Hook pit_handler() into IRQ0
}

uint64_t pit_ticks(void) {
    return ticks;
}