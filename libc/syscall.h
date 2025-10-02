#ifndef __SYSCALL_H
#define __SYSCALL_H

void reboot();
int read(char* buf, int len);
int fork();
int exec(const char* path);
int wait(int pid);
void exit(int code);

#endif