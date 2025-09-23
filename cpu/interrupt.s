[BITS 32]
section .text
global isr_common_stub
global irq_common_stub
extern isr_handler
extern irq_handler

isr_common_stub:
    cli                 ; Disable interrupts first
    pusha              ; Push all registers
    
    xor eax, eax       ; Clear eax
    mov ax, ds         ; Save ds segment
    push eax
    
    mov ax, 0x10       ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp           ; Push stack pointer (registers_t* argument)
    call isr_handler
    add esp, 4         ; Remove pushed argument
    
    pop eax            ; Restore original data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa               ; Restore all registers
    add esp, 8         ; Remove error code and interrupt number
    sti                ; Re-enable interrupts
    iret               ; Return from interrupt

irq_common_stub:
    cli
    pusha
    
    xor eax, eax
    mov ax, ds
    push eax
    
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp
    call irq_handler
    add esp, 4
    
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa
    add esp, 8
    sti
    iret
