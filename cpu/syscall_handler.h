#ifndef __SYSCALL_HANDLER_H
#define __SYSCALL_HANDLER_H

#include "isr.h"

#define SYS_REBOOT 1
#define SYS_WRITE  2
#define SYS_READ   3
#define SYS_EXIT   4

void init_syscalls();
void syscall_handler(registers_t* r);

#endif