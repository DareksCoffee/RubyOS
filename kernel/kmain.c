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
#include <../drivers/mouse/mouse.h>
#include <../drivers/usb/hid/keyboard.h>
#include <../drivers/usb/hid/mouse.h>
#include <../user/shell/shell.h>
#include <../drivers/time/pit.h>
#include <../drivers/screen.h>
#include <../cpu/syscall_handler.h>
#include <../libc/syscall.h>
#include <../drivers/usb/usb.h>
#include <../drivers/net/ethernet/rtl8139.h>
#include <../drivers/net/dns.h>
#include <../drivers/ata.h>
#include "../mm/memory.h"
#include "process.h"

static void timer_callback(registers_t* regs __attribute__((unused))) {
    schedule();
}

void kmain(multiboot_info_t* mbd, unsigned int magic __attribute__((unused))) {
    init_screen(mbd);
    init_graphics();
    cmd_init();

    log(LOG_SYSTEM, "Initializing memory management...");
    init_memory_manager(mbd);
    log(LOG_OK, "Memory management initialized");

    log(LOG_SYSTEM, "Initializing processes...");
    init_processes();
    log(LOG_OK, "Processes initialized");

    log(LOG_SYSTEM, "Detecting system memory...");
    detect_memory(mbd);
    print_memory_map();
    printf("\n");

    log(LOG_SYSTEM, "Total usable memory: %lld bytes", get_total_memory());

    log(LOG_SYSTEM, "Initializing ATA...");
    ata_init();
    log(LOG_OK, "ATA initialized");
    
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
    

    log(LOG_OK, "Initializing RTL8139 Ethernet chip driver.");
    rtl8139_setup();

    log(LOG_SYSTEM, "Initializing DNS...");
    dns_init();
    log(LOG_OK, "DNS initialized");

    if(rtl8139_linkup() == RTL_YES) log(LOG_OK, "Connected to the internet.");
    else log(LOG_ERROR, "Not connected to the internet.");
    log(LOG_SYSTEM, "Initializing PIT...");
    pit_init(100);
    log(LOG_OK, "PIT initialized running in 100Hz.");
    register_interrupt_handler(IRQ0, timer_callback);
    
    log(LOG_SYSTEM, "Enabling interrupts...");
    __asm__ __volatile__ ("sti");
    log(LOG_SYSTEM, "Initializing USB...");
    init_usb();
  
    log(LOG_SYSTEM, "Initializing keyboard...");
    init_keyboard();

    log(LOG_SYSTEM, "Initializing mouse...");
    init_mouse();

    log(LOG_SYSTEM, "Initializing usb mouse...");
    usb_mouse_init();
    
    log(LOG_SYSTEM, "Initializing shell...");
    init_shell();


    
    for (;;) {
        __asm__ __volatile__ ("hlt");
    }
}