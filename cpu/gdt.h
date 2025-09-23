#ifndef GDT_H
#define GDT_H

#include <stdio.h>
#include <stdint.h>
#include <../kernel/logger.h>

void init_gdt(void);
void prnt_gdtinfo(void);
#endif
