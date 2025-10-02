#include "../drivers/time/pit.h"
#include "console.h"
#include "screen.h"
#include "../kernel/bg.h"
#include "../mm/memory.h"

static console_t console;
static uint32_t* bg_pixels = NULL;

static void draw_background(void) {
    if (!bg_pixels) {
        bg_pixels = kmalloc(width * height * 4);
        if (!bg_pixels) return;
        char* data = header_data;
        for (int p = 0; p < width * height; ) {
            unsigned char pixel[3];
            HEADER_PIXEL(data, pixel);
            bg_pixels[p++] = 0xFF000000 | (pixel[2] << 16) | (pixel[1] << 8) | pixel[0];
            if (p < width * height) {
                HEADER_PIXEL(data, pixel);
                bg_pixels[p++] = 0xFF000000 | (pixel[2] << 16) | (pixel[1] << 8) | pixel[0];
            }
            if (p < width * height) {
                HEADER_PIXEL(data, pixel);
                bg_pixels[p++] = 0xFF000000 | (pixel[2] << 16) | (pixel[1] << 8) | pixel[0];
            }
        }
    }
    for (uint32_t y = 0; y < screen.height; y++) {
        for (uint32_t x = 0; x < screen.width; x++) {
            uint32_t x_img = (x * width) / screen.width;
            uint32_t y_img = (y * height) / screen.height;
            uint32_t color = bg_pixels[y_img * width + x_img];
            set_pixel(x, y, color);
        }
    }
}

void console_init(void) {
    console.x = 0;
    console.y = 0;
    console.max_x = screen.width / 8;
    console.max_y = screen.height / 16;
    console.color = 0x00FFFFFF;
    console.bg_color = 0x00000000;
    console_clear();
    //logger_log(OK, "Console initialized");
}

void console_clear(void) {
    fill_screen(console.bg_color);
    console.x = 0;
    console.y = 0;
}

static void clear_char_at(uint32_t x, uint32_t y) {
    draw_rect(x * 8, y * 16, 8, 16, console.bg_color);
}

void console_putchar(char c) {
    if (c == '\n') {
        console.x = 0;
        console.y++;
    } else if (c == '\r') {
        console.x = 0;
    } else if (c == '\t') {
        console.x = (console.x + 4) & ~3;
    } else if (c == '\b') {
        if (console.x > 0) {
            console.x--;
            clear_char_at(console.x, console.y);
        } else if (console.y > 0) {
            console.y--;
            console.x = console.max_x - 1;
            clear_char_at(console.x, console.y);
        }
    } else {
        draw_char(console.x * 8, console.y * 16, c, console.color);
        console.x++;
    }
    
    if (console.x >= console.max_x) {
        console.x = 0;
        console.y++;
    }
    
    if (console.y >= console.max_y) {
        console_scroll();
    }
}

void console_puts(const char* str) {
    while (*str) {
        console_putchar(*str);
        str++;
    }
}

void console_draw_rect(uint32_t width, uint32_t height, uint32_t color, int newline) {
    draw_rect(console.x * 8, console.y * 16, width * 8, height * 16, color);
    if (newline) {
        console.x = 0;
        console.y += height;
        if (console.y >= console.max_y) {
            console_scroll();
        }
    } else {
        console.x += width;
        if (console.x >= console.max_x) {
            console.x = 0;
            console.y++;
            if (console.y >= console.max_y) {
                console_scroll();
            }
        }
    }
}

void console_set_color(uint32_t fg, uint32_t bg) {
    console.color = fg;
    console.bg_color = bg;
}

void console_set_cursor(uint32_t x, uint32_t y) {
    console.x = x;
    console.y = y;
}

void console_scroll(void) {
    screen_scroll(console.bg_color);
    console.y = console.max_y - 1;
    console.x = 0;
}

uint32_t console_get_cursor_x(void) {
    return console.x;
}

uint32_t console_get_cursor_y(void) {
    return console.y;
}

static void print_int(int num, int base) {
    char digits[] = "0123456789ABCDEF";
    char buffer[32];
    int i = 0;
    int negative = 0;
    unsigned int n = num;

    if (num < 0 && base == 10) {
        negative = 1;
        n = -num;
    }
    
    if (n == 0) {
        console_putchar('0');
        return;
    }
    
    while (n > 0) {
        buffer[i++] = digits[n % base];
        n /= base;
    }
    
    if (negative) {
        console_putchar('-');
    }
    
    while (i > 0) {
        console_putchar(buffer[--i]);
    }
}

static void console_print_padded_string(const char* str, int width, char pad) {
    int len = 0;
    const char* s = str;
    while (*s++) len++;
    for (int i = 0; i < width - len; i++) console_putchar(pad);
    console_puts(str);
}
static void console_print_uint(unsigned int num, int base, int width, char pad) {
    const char* digits = "0123456789ABCDEF";
    char buffer[32];
    int i = 0;

    if (num == 0) {
        buffer[i++] = '0';
    } else {
        while (num > 0) {
            buffer[i++] = digits[num % base];
            num /= base;
        }
    }

    // padding
    while (i < width) {
        console_putchar(pad);
        width--;
    }

    while (i > 0) {
        console_putchar(buffer[--i]);
    }
}

static void console_print_ulonglong(unsigned long long num, int base, int width, char pad) {
    const char* digits = "0123456789ABCDEF";
    char buffer[64];
    int i = 0;

    if (num == 0) {
        buffer[i++] = '0';
    } else {
        unsigned long long n = num;
        while (n != 0) {
            unsigned long long digit = n;
            n = n / base;
            digit = digit - (n * base);
            buffer[i++] = digits[digit];
        }
    }

    while (i < width) {
        console_putchar(pad);
        width--;
    }

    while (i > 0) {
        console_putchar(buffer[--i]);
    }
}

static void console_print_padded_int(int value, int width) {
    char buffer[32];
    int i = 0;
    int n = value;
    if (value == 0) buffer[i++] = '0';
    while (n > 0) {
        buffer[i++] = '0' + (n % 10);
        n /= 10;
    }
    // reverse
    for (int j = i; j < width; j++) console_putchar(' ');
    while (i > 0) console_putchar(buffer[--i]);
}

void console_vprintf(const char* format, va_list args) {
    while (*format) {
        if (*format == '%') {
            format++;
            int width = 0;
            int precision = -1;
            char pad = ' ';
            int is_longlong = 0;

            if (*format == '0') {
                pad = '0';
                format++;
            }

            while (*format >= '0' && *format <= '9') {
                width = width * 10 + (*format - '0');
                format++;
            }

            if (*format == '.') {
                format++;
                if (*format == '*') {
                    precision = va_arg(args, int);
                    format++;
                } else {
                    precision = 0;
                    while (*format >= '0' && *format <= '9') {
                        precision = precision * 10 + (*format - '0');
                        format++;
                    }
                }
            }

            if (*format == 'l' && *(format + 1) == 'l') {
                is_longlong = 1;
                format += 2;
            }

            switch (*format) {
                case 'u':
                    if (is_longlong) {
                        console_print_ulonglong(va_arg(args, unsigned long long), 10, width, pad);
                    } else {
                        console_print_uint(va_arg(args, unsigned int), 10, width, pad);
                    }
                    break;
                case 'd':
                    console_print_uint((unsigned int)va_arg(args, int), 10, width, pad);
                    break;
                case 'x':
                case 'X':
                    console_print_uint(va_arg(args, unsigned int), 16, width, pad);
                    break;
                case 's':
                    {
                        char* str = va_arg(args, char*);
                        if (precision >= 0) {
                            // Print with precision
                            int len = 0;
                            while (str[len] && len < precision) len++;
                            for (int i = 0; i < width - len; i++) console_putchar(pad);
                            for (int i = 0; i < len; i++) console_putchar(str[i]);
                        } else {
                            console_print_padded_string(str, width, pad);
                        }
                    }
                    break;
                case 'c':
                    console_putchar(va_arg(args, int));
                    break;
                case '%':
                    console_putchar('%');
                    break;
                default:
                    console_putchar('%');
                    if (is_longlong) {
                        console_putchar('l');
                        console_putchar('l');
                    }
                    console_putchar(*format);
                    break;
            }
        } else {
            console_putchar(*format);
        }
        format++;
    }
}

void console_print_kv(const char* key, const char* value, int key_width, int value_width) {
    int i;
    for (i = 0; key[i] && i < key_width; i++) console_putchar(key[i]);
    for (; i < key_width; i++) console_putchar(' ');

    console_putchar(':');
    console_putchar(' ');

    for (i = 0; value[i] && i < value_width; i++) console_putchar(value[i]);
    for (; i < value_width; i++) console_putchar(' ');

    console_putchar('\n');
}

void console_print_kv_int(const char* key, int value, int key_width, int value_width) {
    char buffer[32];
    int i = 0;
    int n = value;
    if (n == 0) buffer[i++] = '0';
    else {
        while (n > 0) {
            buffer[i++] = '0' + (n % 10);
            n /= 10;
        }
    }

    char str[32];
    int j = 0;
    while (i > 0) str[j++] = buffer[--i];
    str[j] = '\0';

    console_print_kv(key, str, key_width, value_width);
}
void console_print_animated(const char* str, uint32_t* colors, int num_colors, int interval_ms) {
    int len = 0;
    while (str[len]) len++;
    for (int offset = -len; offset <= 0; offset++) {
        console_set_cursor(0, console.y);
        for (int i = 0; i < console.max_x; i++) {
            int char_index = i - offset;
            if (char_index >= 0 && char_index < len) {
                console_set_color(colors[char_index % num_colors], 0x00000000);
                console_putchar(str[char_index]);
            } else {
                console_set_color(0x00FFFFFF, 0x00000000);
                console_putchar(' ');
            }
        }
        uint64_t start_ticks = pit_ticks();
        while (pit_ticks() - start_ticks < (interval_ms / 10));
    }
    console_set_cursor(0, console.y + 1);
}


void console_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    console_vprintf(format, args);
    va_end(args);
}