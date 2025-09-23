BITS 32

section .text

global syscall

syscall:
    ; eax contains the syscall number
    ; ebx, ecx, edx, esi, edi contain the arguments
    int 0x80
    ret