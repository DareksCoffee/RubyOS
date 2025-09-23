# Makefile for RubyOS

# Programs
ASM = nasm
CC = gcc
LD = ld
GRUB_MKRESCUE = grub-mkrescue

# Flags
ASMFLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -c -g -Wall -Wextra -Idrivers -Ilibc -Ikernel -Icpu -Iuser -fno-stack-protector 
LDFLAGS = -m elf_i386 -T linker.ld
GRUB_FLAGS = -o bin/rubyos.iso build/isofiles

# Files
C_SOURCES = $(shell find kernel drivers libc cpu user -name '*.c')
ASM_SOURCES = $(shell find boot cpu -name '*.s')
C_OBJECTS = $(patsubst %.c, build/%.o, $(C_SOURCES))
ASM_OBJECTS = $(patsubst %.s, build/%_asm.o, $(ASM_SOURCES))
OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS)
KERNEL = build/kernel.elf

.PHONY: all clean iso run

all: iso

iso: $(KERNEL)
	mkdir -p bin
	mkdir -p build/isofiles/boot/grub
	cp boot/grub/grub.cfg build/isofiles/boot/grub/grub.cfg
	cp $(KERNEL) build/isofiles/boot/kernel.elf
	$(GRUB_MKRESCUE) $(GRUB_FLAGS)

$(KERNEL): $(OBJECTS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

build/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

build/%_asm.o: %.s
	mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -rf build bin

run: iso
	qemu-system-x86_64 -cdrom bin/rubyos.iso