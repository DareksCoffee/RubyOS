#include "rsh.h"
#include <stdio.h>
#include <string.h>
#include <../fs/rfss.h>
#include <../libc/syscall.h>
#include <../libc/stdio.h>
#include <../mm/memory.h>
#include <../user/shell/commands.h>

void execute_script(char* script, char** script_args, int arg_count);
char* trim(char* str);
int starts_with(char* str, char* prefix);

void execute_rsh_script(const char* filename, char** script_args, int arg_count) {
    rfss_fs_t* fs = rfss_get_mounted_fs();
    if (!fs || !fs->mounted) {
        printf("No filesystem mounted\n");
        return;
    }
    rfss_file_t file;
    if (rfss_open_file(fs, filename, 0, &file) != 0) {
        printf("Cannot open file %s\n", filename);
        return;
    }
    char buffer[4096];
    int len = rfss_read_file(&file, (uint8_t*)buffer, sizeof(buffer) - 1);
    buffer[len] = '\0';
    rfss_close_file(&file);
    execute_script(buffer, script_args, arg_count);
}

void execute_script(char* script, char** script_args, int arg_count) {
    char* p = script;
    int in_if = 0;
    int execute_if = 0;
    while (*p) {
        char line[256];
        int i = 0;
        while (*p && *p != '\n' && i < 255) {
            line[i++] = *p++;
        }
        line[i] = '\0';
        if (*p == '\n') p++;
        char* trimmed = trim(line);
        if (strcmp(trimmed, "if true") == 0) {
            in_if = 1;
            execute_if = 1;
        } else if (strcmp(trimmed, "else") == 0) {
            if (in_if) {
                execute_if = !execute_if;
            }
        } else if (strcmp(trimmed, "endif") == 0) {
            in_if = 0;
            execute_if = 0;
        } else if (starts_with(trimmed, "echo ")) {
            if (!in_if || execute_if) {
                char* msg = trimmed + 5;
                if (msg[0] == '"') {
                    msg++;
                    char* end = strchr(msg, '"');
                    if (end) *end = '\0';
                }
                // Expand variables
                char expanded[256];
                int j = 0;
                for (int k = 0; msg[k] && j < 255; k++) {
                    if (msg[k] == '$' && msg[k+1] >= '1' && msg[k+1] <= '9') {
                        int idx = msg[k+1] - '1';
                        if (idx < arg_count && script_args[idx]) {
                            strcpy(expanded + j, script_args[idx]);
                            j += strlen(script_args[idx]);
                        }
                        k++; // skip the number
                    } else {
                        expanded[j++] = msg[k];
                    }
                }
                expanded[j] = '\0';
                printf("%s\n", expanded);
            }
        } else {
            if (!in_if || execute_if) {
                shell_execute_command(trimmed);
            }
        }
    }
}

char* trim(char* str) {
    while (*str == ' ') str++;
    char* end = str + strlen(str) - 1;
    while (end > str && *end == ' ') *end-- = '\0';
    return str;
}

int starts_with(char* str, char* prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}