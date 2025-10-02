#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stdarg.h>

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t max_x;
    uint32_t max_y;
    uint32_t color;
    uint32_t bg_color;
} console_t;

void console_init(void);
void console_clear(void);
void console_putchar(char c);
void console_puts(const char* str);
void console_draw_rect(uint32_t width, uint32_t height, uint32_t color, int newline);
void console_printf(const char* format, ...);
void console_vprintf(const char* format, va_list args);
void console_set_color(uint32_t fg, uint32_t bg);
void console_set_cursor(uint32_t x, uint32_t y);
void console_scroll(void);

uint32_t console_get_cursor_x(void);
uint32_t console_get_cursor_y(void);


void console_print_animated(const char* str, uint32_t* colors, int num_colors, int interval_ms);
void console_print_kv(const char* key, const char* value, int key_width, int value_width);
void console_print_kv_int(const char* key, int value, int key_width, int value_width);
#endif