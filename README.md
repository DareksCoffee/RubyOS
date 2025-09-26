# RubyOS

**Version:** RAE-0.0.2a

A hobby operating system built from scratch with a focus on learning and exploring low-level system development.

---

## Screenshot

![RubyOS Screenshot](https://i.imgur.com/X3uJV9S.png)

---

## Features

RubyOS is a 32-bit operating system that boots into a graphical desktop environment and provides a basic interactive environment.

### Core Kernel

*   **Bootloader:** Multiboot compliant, designed to be loaded by GRUB.
*   **32-bit Protected Mode:** Establishes a proper protected mode environment with a Global Descriptor Table (GDT) defining kernel (Ring 0) and user (Ring 3) segments.
*   **Interrupt Handling:** A full Interrupt Descriptor Table (IDT) is configured to manage hardware and software interrupts.
    *   **ISRs:** Handles critical CPU exceptions (e.g., Division by Zero, General Protection Fault) to ensure system stability.
    *   **IRQs:** The Programmable Interrupt Controller (PIC) is remapped and handlers are in place for hardware interrupts.
*   **System Timer:** The Programmable Interval Timer (PIT) is initialized to provide a consistent 100Hz system tick, laying the groundwork for future multitasking.
*   **System Information:** Can retrieve and display detailed CPU information (Model, Vendor, Features, Thread Count) using the `cpuid` instruction.

### Drivers

*   **Graphical Console:**
    *   Initializes the display in a high-resolution graphical mode (e.g., 1920x1080x32).
    *   Provides a console interface with support for colored text, automatic scrolling, and character rendering using a built-in 8x16 font.
*   **PS/2 Keyboard Driver:**
    *   Handles keyboard input via IRQ1.
    *   Translates hardware scancodes into ASCII characters.
    *   Supports modifier keys like `Shift` and `Caps Lock`.
*   **ATA (PATA/IDE) Driver:**
    *   A basic PIO (Programmed I/O) mode driver for ATA hard drives.
    *   Includes functionality for drive identification and reading/writing raw sectors.
*   **USB Driver:**
    *   Supports USB controllers (UHCI) with device enumeration and management.
    *   Includes HID (Human Interface Device) support for keyboards and other devices.
*   **PS/2 Mouse Driver:**
    *   Handles mouse input with position tracking and button states.
    *   Supports callback mechanisms for mouse events.
*   **VESA Graphics Driver:**
    *   Enables high-resolution graphical modes for advanced display capabilities.
*   **Framebuffer Driver:**
    *   Manages graphics framebuffer for rendering and display operations.

### System & Userspace

*   **Interactive Shell:**
    *   A functional command-line interface that starts after boot.
    *   Features a customizable, themed prompt with color support.
    *   Parses user input to execute built-in commands.
*   **System Logger:** A kernel-level logging facility that outputs color-coded messages for different severity levels (e.g., System, OK, Error) to the console.
*   **Desktop Environment:**
    *   Graphical desktop with mouse cursor support and an event-driven interface.
    *   Provides a basic GUI framework for user interaction.
*   **RFSS Filesystem:**
    *   A custom journaling filesystem (Ruby File System) with support for files, directories, symlinks, and devices.
    *   Features extents for efficient storage, inode management, and filesystem integrity checks.
*   **Memory Management:**
    *   Dynamic memory allocation with kmalloc and kfree functions.
    *   Memory map detection and statistics tracking.
*   **System Calls:**
    *   Basic syscall interface for kernel-user communication (e.g., reboot).
*   **I/O Ports:**
    *   Low-level port I/O operations for hardware interaction.
*   **Minimal Standard Library (Libc):** Includes essential functions for string manipulation (`strlen`, `strcpy`, `strcmp`), memory operations (`memset`, `memcpy`), formatted output (`printf`), and random number generation (`srand_seed`).

---


## Building and Running



### Prerequisites

*   A cross-compiler for `i686-elf`.
*   `make`
*   `grub-mkrescue`
*   `qemu-system-i386`

### Build Steps

```bash
# 1. Compile the kernel and userspace programs
make all

# 2. Run in QEMU
make run
```

---
