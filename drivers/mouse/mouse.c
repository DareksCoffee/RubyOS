#include "mouse.h"
#include <../cpu/ports.h>
#include <../cpu/isr.h>
#include <../cpu/irq.h>
#include <logger.h>
#include <stddef.h>

static mouse_state_t mouse_state = {0, 0, 0};
static void (*mouse_callback)(mouse_state_t state) = NULL;
static uint8_t mouse_cycle = 0;
static int8_t mouse_bytes[3];

static void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
    }
}

static void mouse_write(uint8_t data) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, data);
}

static uint8_t mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}

static void mouse_callback_handler(registers_t* regs __attribute__((unused))) {
    uint8_t data = inb(0x60);
    switch (mouse_cycle) {
        case 0:
            mouse_bytes[0] = data;
            if ((data & 0x08) == 0) break;
            mouse_cycle++;
            break;
        case 1:
            mouse_bytes[1] = data;
            mouse_cycle++;
            break;
        case 2:
            mouse_bytes[2] = data;
            mouse_state.buttons = mouse_bytes[0] & 0x07;
            mouse_state.x += mouse_bytes[1];
            mouse_state.y -= mouse_bytes[2];
            if (mouse_state.x < 0) mouse_state.x = 0;
            if (mouse_state.y < 0) mouse_state.y = 0;
            if (mouse_callback) mouse_callback(mouse_state);
            mouse_cycle = 0;
            break;
    }
}

void init_mouse(void) {
    uint8_t status;
    mouse_wait(1);
    outb(0x64, 0xAD);
    mouse_wait(1);
    outb(0x64, 0xA8);
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    status = (inb(0x60) | 2) & ~0x20;
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);
    mouse_write(0xF6);
    mouse_read();
    mouse_write(0xF4);
    mouse_read();
    mouse_wait(1);
    outb(0x64, 0xAE);
    register_interrupt_handler(IRQ12, mouse_callback_handler);
    log(LOG_OK, "Mouse initialized");
}

mouse_state_t get_mouse_state(void) {
    return mouse_state;
}

void mouse_set_callback(void (*callback)(mouse_state_t state)) {
    mouse_callback = callback;
}