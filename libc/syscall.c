#include "syscall.h"

extern int syscall(int eax, int ebx, int ecx, int edx);

void reboot() {
    syscall(0, 0, 0, 0);
}

int read(char* buf, int len) {
    return syscall(2, (int)buf, len, 0);
}

int fork() {
    return syscall(4, 0, 0, 0);
}

int exec(const char* path) {
    return syscall(5, (int)path, 0, 0);
}

int wait(int pid) {
    return syscall(6, pid, 0, 0);
}

void exit(int code) {
    syscall(3, code, 0, 0);
}