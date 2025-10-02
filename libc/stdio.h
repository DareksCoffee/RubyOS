#ifndef STDIO_H
#define STDIO_H

#include <../drivers/console.h>

#define printf console_printf
#define putchar console_putchar
#define puts console_puts

void cmd_init(void);
int sscanf(const char* str, const char* format, ...);
int getchar(void);

#endif