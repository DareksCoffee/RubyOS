#include "stdio.h"
#include "../kernel/logger.h"
#include "string.h"
#include <stdarg.h>

void cmd_init(void) {
    console_init();
    //logger_log(OK, "STDIO initialized");
}

int sscanf(const char* str, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int count = 0;
    const char* s = str;
    const char* f = format;
    
    while (*f && *s) {
        if (*f == '%') {
            f++;
            if (*f == 's') {
                char* dest = va_arg(args, char*);
                while (*s && *s != ' ' && *s != '\t' && *s != '\n') {
                    *dest++ = *s++;
                }
                *dest = '\0';
                count++;
            } else if (*f == 'd') {
                int* dest = va_arg(args, int*);
                *dest = 0;
                int sign = 1;
                if (*s == '-') {
                    sign = -1;
                    s++;
                }
                while (*s >= '0' && *s <= '9') {
                    *dest = *dest * 10 + (*s - '0');
                    s++;
                }
                *dest *= sign;
                count++;
            }
            f++;
        } else if (*f == ' ') {
            while (*s == ' ' || *s == '\t') s++;
            f++;
        } else {
            if (*f != *s) break;
            f++;
            s++;
        }
    }
    
    va_end(args);
    return count;
}