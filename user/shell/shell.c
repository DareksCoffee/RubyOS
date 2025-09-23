#include "shell.h"
#include "commands.h"
#include <logger.h>
#include <stdio.h>
#include <info.h>
#include <string.h>
#include <field_map.h>
#include <../drivers/keyboard/keyboard.h>
#include <../cpu/ports.h>


static void cmd_help(const char* args);
static void cmd_clear(const char* args);
static void cmd_vian(const char* args);
static void cmd_info(const char* args);
static void cmd_echo(const char* args);
static void cmd_setkeys(const char* args);
static void cmd_reboot(const char* args);

static const command_t commands[] = {
    {"help", "Display this help message", cmd_help},
    {"vian", "Baaah", cmd_vian},
    {"clear", "Clear the screen", cmd_clear},
    {"info", "Display information about the system", cmd_info},
    {"echo", "Repeats your input", cmd_echo},
    {"setkeys", "Set keyboard layout. Use -ls to list layouts.", cmd_setkeys},
    {"reboot", "Reboot the system.", cmd_reboot},
    {NULL, NULL, NULL}
};

static input_callback_t input_callback = NULL;

void init_shell(void) {
    log(LOG_OK, "Shell initialized");
    shell_display_prompt();
}

void shell_set_input_callback(input_callback_t callback) {
    input_callback = callback;
}

void shell_handle_input(const char* input) {
    if (!input || !*input) {
        shell_display_prompt();
        return;
    }

    char cmd_name[32];
    int i;
    for (i = 0; input[i] && input[i] != ' ' && i < 31; i++) {
        cmd_name[i] = input[i];
    }
    cmd_name[i] = '\0';
    
    for (const command_t* command = commands; command->name != NULL; command++) {
        if (strcmp(cmd_name, command->name) == 0) {
            command->handler(input + i + (input[i] ? 1 : 0));
            shell_display_prompt();
            return;
        }
    }
    
    printf("Unknown command: %s\n", cmd_name);
    shell_display_prompt();
}

void shell_display_prompt(void) {
    printf("[ ");
    console_set_color(0x00FF0000, 0x00000000);  
    printf("RubyOS");
    console_set_color(0x00FFFFFF, 0x00000000); 
    printf(" ]");
    printf(" [~]");
    printf(" -$ ");

}

static void cmd_help(const char* args __attribute__((unused))) {
    printf("Available commands:\n");
    for (const command_t* cmd = commands; cmd->name != NULL; cmd++) {
        printf("  %s - %s\n", cmd->name, cmd->description);
    }
}

static void cmd_clear(const char* args __attribute__((unused))) {
    console_clear();
}

static void cmd_info(const char* args) {
    system_info_t info;
    get_system_info(&info);

    field_map_t fields[] = {
        {"os_name", FIELD_STRING, info.os_name},
        {"version", FIELD_STRING, info.version},
        {"architecture", FIELD_STRING, info.architecture},
        {"cpu_model", FIELD_STRING, info.cpu_model},
        {"cpu_threads", FIELD_INT, &info.cpu_threads},
        {"cpu_features", FIELD_INT, &info.cpu_features},
    };

    int count = sizeof(fields) / sizeof(fields[0]);

    // Print full system info if no args
    if (!args || !*args) {
        printf("========================================\n");
        printf("|");
        console_set_color(0x00FF0000, 0x00000000);
        printf("          System Information          ");
        console_set_color(0x00FFFFFF, 0x00000000);
        printf("|");
        printf("\n");
        printf("========================================\n");

        for (int i = 0; i < count; i++) {
            if (fields[i].type == FIELD_STRING)
                console_print_kv(fields[i].name, (char*)fields[i].ptr, 14, 28);
            else
                console_print_kv_int(fields[i].name, *(int*)fields[i].ptr, 14, 28);
        }

        printf("========================================\n");
        return;
    }

    // Print only the requested field
    for (int i = 0; i < count; i++) {
        if (strcmp(args, fields[i].name) == 0) {
            if (fields[i].type == FIELD_STRING)
                console_print_kv(fields[i].name, (char*)fields[i].ptr, 14, 28);
            else
                console_print_kv_int(fields[i].name, *(int*)fields[i].ptr, 14, 28);
            return;
        }
    }

    printf("Unknown info field: %s\n", args);
}

static void cmd_echo(const char* args) {
    const char *start = strchr(args, '"');
    const char *end   = strrchr(args, '"');

    if (start && end && start != end) {
        start++;
        while (start < end) putchar(*start++);
        putchar('\n');
    } else printf("%s\n", args);
    
}
static void cmd_vian(const char* args) {
    if (!args || !*args) {
        args = "Baaah!";
    }

    int len = strlen(args);
    printf(" ");
    for (int i = 0; i < len + 2; i++) printf("_");
    printf("\n");

    printf("< %s >\n", args);

    printf(" ");
    for (int i = 0; i < len + 2; i++) printf("-");
    printf("\n");

    printf("         ,ww\n");
    printf("   wWWWWWWW_)\n");
    printf("   `WWWWWW'\n");
    printf("    II  II\n");
}

static void cmd_setkeys(const char* args) {
    if (strcmp(args, "list") == 0) {
        keyboard_list_layouts();
    } else {
        keyboard_set_layout(args);
    }
}


static void cmd_reboot(const char* args)
{
    log(LOG_SYSTEM, "Rebooting..");
    outb(0x64, 0xFE);
}