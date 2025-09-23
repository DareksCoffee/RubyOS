#include <../drivers/multi_boot.h>
#include <../drivers/screen.h>
#include <../cpu/isr.h>
#include <../cpu/irq.h>
#include <../cpu/gdt.h>
#include <../cpu/idt.h>
#include <logger.h>
#include <stdio.h>
#include <stdint.h>
#include <../drivers/keyboard/keyboard.h>
#include <../user/shell/shell.h>
#include <../drivers/time/pit.h>
#include <../cpu/syscall_handler.h>
#include <../libc/syscall.h>

static void timer_callback(registers_t* regs __attribute__((unused))) {
    /*
        Empty for now
    */
}

void kmain(multiboot_info_t* mbd, unsigned int magic __attribute__((unused))) {
    init_screen(mbd);
    init_graphics();
    cmd_init();
    
    log(LOG_SYSTEM, "Initializing GDT...");
    init_gdt();
    prnt_gdtinfo();
    log(LOG_OK, "GDT Initialized.");
    __asm__ __volatile__ ("cli");
    
    log(LOG_SYSTEM, "Initializing ISRs...");
    init_isr();  
    log(LOG_OK, "ISR initialized.");
    
    log(LOG_SYSTEM, "Initializing IRQs...");
    init_irq();
    log(LOG_OK, "IRQs initialized.");

    log(LOG_SYSTEM, "Initializing syscalls...");
    init_syscalls();
    log(LOG_OK, "Syscalls initialized.");
    
    log(LOG_SYSTEM, "Initializing PIT...");
    pit_init(100);
    log(LOG_OK, "PIT initialized running in 100Hz.");
    register_interrupt_handler(IRQ0, timer_callback);
    
    log(LOG_SYSTEM, "Enabling interrupts...");
    __asm__ __volatile__ ("sti");
    
    log(LOG_SYSTEM, "Initializing keyboard...");
    init_keyboard();
    
    log(LOG_SYSTEM, "Initializing shell...");
    init_shell();
    keyboard_set_callback(shell_handle_input);


    
    for (;;) {
        __asm__ __volatile__ ("hlt");
    }
}