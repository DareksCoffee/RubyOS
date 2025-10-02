#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

#define CMD_SAFE 0
#define CMD_UNSAFE 1
#define CMD_MAINTENANCE 2

typedef struct command {
    const char* name;
    const char* description;
    void (*handler)(const char* args);
    int safety;
} command_t;

void shell_execute_command(const char* input);

#endif
