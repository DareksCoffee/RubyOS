#ifndef __INFO_H__
#define __INFO_H__

typedef struct {
    char os_name[32];
    char version[16];
    char architecture[16];
    char cpu_model[48];   
    char cpu_vendor[16];
    int cpu_cores;        
    int cpu_threads;      
    unsigned int cpu_features; 
    int uptime_seconds;
} system_info_t;


static void get_cpuid(unsigned int eax_in, unsigned int* eax_out, unsigned int* ebx_out, unsigned int* ecx_out, unsigned int* edx_out);
void get_system_info(system_info_t* info);

#endif // __INFO_H__
