#include "process.h"
#include <../mm/memory.h>
#include <../kernel/logger.h>
#include <string.h>

static process_t* process_list = NULL;
process_t* current_process = NULL;
static uint32_t next_pid = 1;

void init_processes() {
    process_list = NULL;
    current_process = NULL;
    next_pid = 1;
}

process_t* create_process(void (*entry)()) {
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    if (!proc) return NULL;

    proc->pid = next_pid++;
    proc->state = PROCESS_READY;
    memset(&proc->regs, 0, sizeof(registers_t));
    proc->esp = (uint32_t)(proc->stack + STACK_SIZE);
    proc->ebp = proc->esp;
    if (entry) {
        proc->eip = (uint32_t)entry;
        proc->regs.eip = (uint32_t)entry;
    }
    proc->regs.esp = proc->esp;
    proc->regs.ebp = proc->ebp;
    proc->next = process_list;
    process_list = proc;

    log(LOG_SYSTEM, "Created process %d", proc->pid);
    return proc;
}

void schedule() {
    if (!process_list) return;

    process_t* next = current_process ? current_process->next : process_list;
    if (!next) next = process_list;

    if (current_process && current_process->state == PROCESS_RUNNING) {
        current_process->state = PROCESS_READY;
    }

    next->state = PROCESS_RUNNING;
    if (current_process != next) {
        context_switch(next);
    }
    current_process = next;
}

void context_switch(process_t* next) {
    if (!current_process) {
        __asm__ __volatile__(
            "mov %0, %%esp\n"
            "mov %1, %%ebp\n"
            "jmp *%2\n"
            :
            : "r"(next->esp), "r"(next->ebp), "r"(next->eip)
        );
        return;
    }

    __asm__ __volatile__(
        "pusha\n"
        "pushf\n"
        "mov %%esp, %0\n"
        "mov %%ebp, %1\n"
        "mov %2, %%esp\n"
        "mov %3, %%ebp\n"
        "popf\n"
        "popa\n"
        "jmp *%4\n"
        : "=m"(current_process->esp), "=m"(current_process->ebp)
        : "m"(next->esp), "m"(next->ebp), "m"(next->eip)
    );
}

void yield() {
    schedule();
}