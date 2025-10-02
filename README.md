# RubyOS

**Version:** RAE-0.0.3a

A hobby operating system built from scratch with a focus on learning and exploring low-level system development.

---

## Screenshots

![RubyOS Message](https://i.imgur.com/CDotXrC.png)
*Showcase of rsh*

---

![RubyOS Help](https://i.imgur.com/K69zqaT.png)
*Showcase of the help command*

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
*   **Multitasking:** Basic process management with scheduling, context switching, process creation, and yield functionality.
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
*   **Network Drivers:**
    *   Ethernet Driver (RTL8139): Supports wired networking with packet transmission and reception.
    *   Network Device Abstraction: Provides a unified interface for network devices with init, send, and poll operations.
    *   Protocol Support: Implements ARP, DNS, ICMP, IP, and UDP protocols for network communication.

### System & Userspace

*   **Interactive Shell:**
    *   A functional command-line interface that starts after boot.
    *   Features a customizable, themed prompt with color support.
    *   Parses user input to execute built-in commands.
    *   Includes a built-in text editor for editing files within the shell environment.
*   **System Logger:** A kernel-level logging facility that outputs color-coded messages for different severity levels (e.g., System, OK, Error) to the console.
*   **Desktop Environment:**
    *   Graphical desktop with mouse cursor support and an event-driven interface.
    *   Provides a basic GUI framework for user interaction.
*   **RFSS+ Filesystem:**
    *   A custom journaling filesystem (Ruby File System Signature) with support for files, directories, symlinks, and devices.
    *   Features extents for efficient storage, inode management, and filesystem integrity checks.
    * *IMPORTANT NOTE*: RFSS+ Jounraling System is disabled by default in the latest version due to numerous issues that will be patched in future updates.
*   **Memory Management:**
    *   Dynamic memory allocation with kmalloc and kfree functions.
    *   Memory map detection and statistics tracking.
*   **System Calls:**
    *   Basic syscall interface for kernel-user communication (e.g., reboot).
*   **I/O Ports:**
    *   Low-level port I/O operations for hardware interaction.
*   **Minimal Standard Library (Libc):** Includes essential functions for string manipulation (`strlen`, `strcpy`, `strcmp`), memory operations (`memset`, `memcpy`), formatted output (`printf`), random number generation (`srand_seed`), and MD5 hashing.

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

## Testing

RubyOS includes a comprehensive test suite written in Python to validate various components including the kernel, drivers, filesystem, libc, and UI.

```bash
# Run the test suite
python tests/run_tests.py
```

---
