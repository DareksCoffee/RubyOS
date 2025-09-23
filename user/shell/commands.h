#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

typedef struct command {
    const char* name;
    const char* description;
    void (*handler)(const char* args);
} command_t;

#endif
