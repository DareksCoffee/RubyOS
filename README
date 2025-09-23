# RubyOS

**Version:** RAE-0.0.1a

A hobby operating system built from scratch with a focus on learning and exploring low-level system development.

---

## Screenshot

![RubyOS Screenshot](https://i.imgur.com/KmnZJ4a.png)

---

## Features

RubyOS is a 32-bit operating system that boots into a graphical console and provides a basic interactive environment.

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

### System & Userspace

*   **Interactive Shell:**
    *   A functional command-line interface that starts after boot.
    *   Features a customizable, themed prompt.
    *   Parses user input to execute built-in commands.
*   **System Logger:** A kernel-level logging facility that outputs color-coded messages for different severity levels (e.g., System, OK, Error) to the console.
*   **Minimal Standard Library (Libc):** Includes essential functions for string manipulation (`strlen`, `strcpy`, `strcmp`), memory operations (`memset`, `memcpy`), and formatted output (`printf`).

---

## 🛠️ Building and Running



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
