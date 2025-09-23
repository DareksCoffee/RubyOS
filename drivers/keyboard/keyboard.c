#include "keyboard.h"
#include "scancode.h"
#include <../cpu/ports.h>
#include <../cpu/isr.h>
#include <../cpu/irq.h>
#include <logger.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

static uint8_t shift_pressed = 0;
static uint8_t caps_lock = 0;
static char buffer[KEYBOARD_BUFFER_SIZE];
static uint32_t buffer_pos = 0;
static uint32_t input_start = PROMPT_SIZE;
static keyboard_layout_t current_layout = KBD_LAYOUT_US;

static void keyboard_callback(registers_t* regs __attribute__((unused))) {
    uint8_t scancode = inb(0x60);
    
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
    } else if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
    } else if (scancode == 0x3A) {
        caps_lock = !caps_lock;
    } else if (!(scancode & KEY_RELEASE)) {
        if (scancode < 128) {
            char c;
            if (shift_pressed || caps_lock) {
                c = keyboard_layouts[current_layout].shift_map[scancode];
            } else {
                c = keyboard_layouts[current_layout].map[scancode];
            }
            
            if (c) {
                keyboard_input_char(c);
            }
        }
    }
}

void keyboard_set_layout(const char* layout_name) {
    for (int i = 0; i < KBD_LAYOUT_COUNT; i++) {
        if (strcmp(layout_name, keyboard_layouts[i].name) == 0) {
            current_layout = i;
            return;
        }
    }
    printf("Unknown layout: %s\n", layout_name);
}

void keyboard_list_layouts() {
    printf("Available layouts:\n");
    for (int i = 0; i < KBD_LAYOUT_COUNT; i++) {
        printf("  %s\n", keyboard_layouts[i].name);
    }
}

static void (*input_callback)(const char* input) = NULL;

void keyboard_set_callback(void (*callback)(const char* input)) {
    input_callback = callback;
}

void keyboard_input_char(char c) {
    if (c == '\b') {
        keyboard_handle_backspace();
    } else if (c == '\n') {
        buffer[buffer_pos] = '\0';
        putchar('\n');
        if (input_callback) {
            input_callback(buffer);
        }
        buffer_pos = 0;
        input_start = PROMPT_SIZE;
    } else if (buffer_pos < KEYBOARD_BUFFER_SIZE - 2) { 
        buffer[buffer_pos++] = c;
        buffer[buffer_pos] = '\0';
        putchar(c);
    }
}

void keyboard_handle_backspace() {
    if (buffer_pos > 0 && console_get_cursor_x() > input_start) {
        buffer_pos--;
        putchar('\b');
        buffer[buffer_pos] = '\0';
    }
}

void init_keyboard() {
    register_interrupt_handler(IRQ1, keyboard_callback);
    keyboard_clear_buffer();
    log(LOG_OK, "Keyboard initialized");
}

char* keyboard_get_buffer() {
    return buffer;
}

void keyboard_clear_buffer() {
    buffer_pos = 0;
    buffer[0] = '\0';
    input_start = PROMPT_SIZE;
}

#define PROMPT_SIZE 8  