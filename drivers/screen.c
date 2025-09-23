#include "screen.h"
#define FONT8x16_IMPLEMENTATION
#include "fonts/font8x16.h"
static screen_t screen;

void init_screen(multiboot_info_t* mbd) {
    if (mbd->flags & (1 << 12)) {
        screen.framebuffer = (uint32_t*)(uintptr_t)mbd->framebuffer_addr;
        screen.width = mbd->framebuffer_width;
        screen.height = mbd->framebuffer_height;
        screen.pitch = mbd->framebuffer_pitch;
        screen.bpp = mbd->framebuffer_bpp;
        //logger_log(OK, "Screen initialized");
    } else {
        //logger_log(ERROR, "Failed to initialize screen (multiboot info not found)");
    }
}

void init_graphics(void) {
    fill_screen(0x00000000);
    //logger_log(OK, "Graphics initialized");
}

void set_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= screen.width || y >= screen.height) return;
    screen.framebuffer[y * (screen.pitch / 4) + x] = color;
}
void fill_screen(uint32_t color) {
    uint32_t total_pixels = (screen.height * screen.pitch) / 4;
    uint32_t* fb = screen.framebuffer;
    
    // Unroll loop for better performance
    uint32_t i = 0;
    uint32_t remainder = total_pixels % 4;
    uint32_t chunks = total_pixels - remainder;
    
    // Process 4 pixels at once
    for (i = 0; i < chunks; i += 4) {
        fb[i] = color;
        fb[i + 1] = color;
        fb[i + 2] = color;
        fb[i + 3] = color;
    }
    
    // Handle remaining pixels
    for (i = chunks; i < total_pixels; i++) {
        fb[i] = color;
    }
}
void draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    for (uint32_t dy = 0; dy < height; dy++) {
        for (uint32_t dx = 0; dx < width; dx++) {
            set_pixel(x + dx, y + dy, color);
        }
    }
}

void draw_char(uint32_t x, uint32_t y, char c, uint32_t color) {
    unsigned char* font_char = font8x16[(unsigned char)c];
    
    for (int row = 0; row < 16; row++) {
        unsigned char font_row = font_char[row];
        for (int col = 0; col < 8; col++) {
            if (font_row & (0x80 >> col)) {
                set_pixel(x + col, y + row, color);
            }
        }
    }
}
void prints(uint32_t x, uint32_t y, const char* str, uint32_t color)
{
    uint32_t current_x = x;
    uint32_t current_y = y;
    
    while (*str) {
        if (*str == '\n') {
            current_x = x;
            current_y += 16;
        } else {
            draw_char(current_x, current_y, *str, color);
            current_x += 8;
        }
        str++;
    }
}

static void memmove32(uint32_t* dest, const uint32_t* src, uint32_t count) {
    if (dest < src) {
        for (uint32_t i = 0; i < count; i++) {
            dest[i] = src[i];
        }
    } else {
        for (uint32_t i = count; i > 0; i--) {
            dest[i-1] = src[i-1];
        }
    }
}

void screen_scroll(uint32_t bg_color) {
    uint32_t row_size = screen.pitch / 4;
    uint32_t char_height = 16;

    // Move all rows up by one char_height
    memmove32(screen.framebuffer,
              screen.framebuffer + row_size * char_height,
              (screen.height - char_height) * row_size);

    // Clear the last char_height rows
    draw_rect(0, screen.height - char_height, screen.width, char_height, bg_color);
}