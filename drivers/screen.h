#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include "multi_boot.h"

typedef struct {
    uint32_t* framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
} screen_t;

void init_screen(multiboot_info_t* mbd);
void init_graphics(void);
void set_pixel(uint32_t x, uint32_t y, uint32_t color);
void fill_screen(uint32_t color);
void draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void draw_char(uint32_t x, uint32_t y, char c, uint32_t color);
void prints(uint32_t x, uint32_t y, const char* str, uint32_t color);
void screen_scroll(uint32_t bg_color);

#endif