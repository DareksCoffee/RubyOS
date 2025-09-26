#include "ports.h"

void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ __volatile__("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outw(uint16_t port, uint16_t value) {
    __asm__ __volatile__("outw %0, %1" : : "a"(value), "Nd"(port));
}

void outsw(uint16_t port, const void* buffer, uint32_t count) {
    asm volatile ("rep outsw" : "+S"(buffer), "+c"(count) : "d"(port) : "memory");
}

void insw(uint16_t port, void* buffer, uint32_t count) {
    asm volatile ("rep insw" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}


void outl(uint16_t port, uint32_t value) {
    __asm__ __volatile__("outl %0, %1" : : "a"(value), "Nd"(port));
}

uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ __volatile__("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void insl(uint16_t port, uint32_t* buffer, uint32_t count) {
    __asm__ __volatile__("rep insl" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}

void outsl(uint16_t port, const uint32_t* buffer, uint32_t count) {
    __asm__ __volatile__("rep outsl" : "+S"(buffer), "+c"(count) : "d"(port) : "memory");
}
