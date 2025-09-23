#include "info.h"
#include "string.h"

static void get_cpuid(unsigned int eax_in, unsigned int* eax_out, unsigned int* ebx_out, unsigned int* ecx_out, unsigned int* edx_out) {
    __asm__ __volatile__("cpuid"
                         : "=a"(*eax_out), "=b"(*ebx_out), "=c"(*ecx_out), "=d"(*edx_out)
                         : "a"(eax_in));
}

void get_system_info(system_info_t* info) {
    strcpy(info->os_name, "RubyOS");
    strcpy(info->version, "1.0.0");
    strcpy(info->architecture, "x86_64");

    unsigned int eax, ebx, ecx, edx;
    char cpu_name[49]; // 48 chars + null terminator
    char cpu_vendor[13]; // 12 chars + null

    // Get CPU vendor string
    get_cpuid(0, &eax, &ebx, &ecx, &edx);
    memcpy(cpu_vendor + 0, &ebx, 4);
    memcpy(cpu_vendor + 4, &edx, 4);
    memcpy(cpu_vendor + 8, &ecx, 4);
    cpu_vendor[12] = '\0';
    strncpy(info->cpu_model, cpu_vendor, sizeof(info->cpu_model)-1); // temporarily store vendor

    // Get full CPU brand string
    for (unsigned int i = 0; i < 3; ++i) {
        get_cpuid(0x80000002 + i, &eax, &ebx, &ecx, &edx);
        memcpy(cpu_name + i*16 + 0, &eax, 4);
        memcpy(cpu_name + i*16 + 4, &ebx, 4);
        memcpy(cpu_name + i*16 + 8, &ecx, 4);
        memcpy(cpu_name + i*16 + 12, &edx, 4);
    }
    cpu_name[48] = '\0';
    strncpy(info->cpu_model, cpu_name, sizeof(info->cpu_model)-1);
    info->cpu_model[sizeof(info->cpu_model)-1] = '\0';

    // Get basic feature flags (EDX from eax=1)
    get_cpuid(1, &eax, &ebx, &ecx, &edx);
    info->cpu_features = edx; // bitmask: FPU, MMX, SSE, SSE2, etc.

    // Get number of logical processors
    info->cpu_threads = (ebx >> 16) & 0xFF; // bits 16-23
}
