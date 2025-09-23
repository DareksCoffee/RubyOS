#include <stdint.h>
#include <../kernel/logger.h>
#include <../drivers/keyboard/keyboard.h>
#include "idt.h"
#include "isr.h"
#include <stdio.h>
#include "ports.h"

int32_t sys_reboot(uint32_t a1, uint32_t a2, uint32_t a3);
int32_t sys_write(uint32_t buf_ptr, uint32_t len, uint32_t unused);
int32_t sys_read(uint32_t buf_ptr, uint32_t len, uint32_t unused);
int32_t sys_exit(uint32_t code, uint32_t u2, uint32_t u3);

void syscall_handler(registers_t* r) {

    uint32_t syscall_num = r->eax;
    uint32_t arg1 = r->ebx;
    uint32_t arg2 = r->ecx;
    uint32_t arg3 = r->edx;

    int32_t ret_val = 0;

    switch (syscall_num) {
        case 0: // sys_reboot
            ret_val = sys_reboot(arg1, arg2, arg3);
            break;
        case 1: // sys_write
            ret_val = sys_write(arg1, arg2, arg3);
            break;
        case 2: // sys_read
            ret_val = sys_read(arg1, arg2, arg3);
            break;
        case 3: // sys_exit
            ret_val = sys_exit(arg1, arg2, arg3);
            break;
        default:
            log(LOG_ERROR, "Unknown syscall: %d", syscall_num);
            ret_val = -1; // Indicate an error
            break;
    }
    r->eax = ret_val; // Store the return value in EAX
}

int32_t sys_reboot(uint32_t a1, uint32_t a2, uint32_t a3) {
    (void)a1; // Cast to void to suppress unused parameter warning
    (void)a2; // Cast to void to suppress unused parameter warning
    (void)a3; // Cast to void to suppress unused parameter warning
    log(LOG_SYSTEM, "Rebooting...");
    outb(0x64, 0xFE);
    return 0;
}

int32_t sys_write(uint32_t buf_ptr, uint32_t len, uint32_t unused) {
    (void)unused; // Cast to void to suppress unused parameter warning
    const char* buf = (const char*)buf_ptr;
    for (uint32_t i = 0; i < len; i++) {
        putchar(buf[i]);
    }
    return len;
}

int32_t sys_read(uint32_t buf_ptr, uint32_t len, uint32_t unused) {
    (void)unused; // Cast to void to suppress unused parameter warning
    char* buf = (char*)buf_ptr;
    uint32_t read_count = 0;
    char* kbuf = keyboard_get_buffer();
    while (read_count < len && kbuf[read_count] != '\0') {
        buf[read_count] = kbuf[read_count];
        read_count++;
    }
    buf[read_count] = '\0';
    return read_count;
}

int32_t sys_exit(uint32_t code, uint32_t u2, uint32_t u3) {
    (void)u2; // Cast to void to suppress unused parameter warning
    (void)u3; // Cast to void to suppress unused parameter warning
    log(LOG_SYSTEM, "Process exited with code %d", code);
    while (1) { __asm__ volatile ("hlt"); }
    return 0; // never reached
}
extern void isr128();
void init_syscalls() {
    register_interrupt_handler(128, syscall_handler);
    set_idt_gate(128, (uint32_t)isr128);
}