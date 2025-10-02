#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <../cpu/isr.h>

#define MAX_PROCESSES 256
#define STACK_SIZE 4096

typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_TERMINATED
} process_state_t;

typedef struct process {
    uint32_t pid;
    process_state_t state;
    registers_t regs;
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    uint8_t stack[STACK_SIZE];
    struct process* next;
} process_t;

void init_processes();
process_t* create_process(void (*entry)());
void schedule();
void context_switch(process_t* next);
void yield();

extern process_t* current_process;

#endif