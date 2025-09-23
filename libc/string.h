#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void* memset(void* buf, int c, size_t len);
int strcmp(const char* s1, const char* s2);
size_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
void* memcpy(void* dest, const void* src, size_t n);

char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
#endif