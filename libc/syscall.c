#include "syscall.h"

extern void syscall(int eax, int ebx, int ecx, int edx);

void reboot() {
    syscall(1, 0, 0, 0); // SYS_REBOOT is 1
}