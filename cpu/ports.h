#ifndef PORTS_H
#define PORTS_H

#include <stdint.h>

void outb(uint16_t port, uint8_t value);
void outw(uint16_t port, uint16_t value);
void outl(uint16_t port, uint32_t value);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);
void insl(uint16_t port, uint32_t* buffer, uint32_t count);
void outsl(uint16_t port, const uint32_t* buffer, uint32_t count);
void outsw(uint16_t port, const void* buffer, uint32_t count);
void insw(uint16_t port, void* buffer, uint32_t count);
#endif
