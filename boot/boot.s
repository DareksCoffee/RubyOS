; Multiboot header
section .multiboot
align 4
    dd 0x1BADB002                          
    dd (1 << 0) | (1 << 1) | (1 << 2)    
    dd -(0x1BADB002 + ((1 << 0) | (1 << 1) | (1 << 2)))  
    

    dd 0        ; header_addr
    dd 0        ; load_addr  
    dd 0        ; load_end_addr
    dd 0        ; bss_end_addr
    dd 0        ; entry_addr
    dd 0        ; mode_type (0 = linear graphics)
    dd 1920     ; width
    dd 1080     ; height
    dd 32       ; depth

global start
extern kmain

section .text
start:
    cli
    mov esp, stack_top
    
    ; Push multiboot parameters
    push eax    ; multiboot magic number
    push ebx    ; multiboot info structure pointer
    
    call kmain
    
.hang:
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 8192
stack_top: