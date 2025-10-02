#include "shell.h"
#include "commands.h"
#include <logger.h>
#include <stdio.h>
#include <info.h>
#include <string.h>
#include <field_map.h>
#include <../drivers/keyboard/keyboard.h>
#include "../drivers/ata.h"
#include <../cpu/ports.h>
#include <../fs/rfss.h>
#include "../ui/desktop/desktop.h"
#include <../libc/syscall.h>
#include <../libc/stdio.h>
#include <../drivers/mouse/mouse.h>
#include <../drivers/usb/usb_device.h>
#include <../drivers/net/ethernet/rtl8139.h>
#include <../drivers/net/icmp.h>
#include <../drivers/net/arp.h>
#include <../drivers/net/dns.h>
#include "../utils/fs_util.h"
#include "edit.h"
#include "rsh/rsh.h"

static void cmd_help(const char* args);
static void cmd_clear(const char* args);
static void cmd_vian(const char* args);
static void cmd_info(const char* args);
static void cmd_echo(const char* args);
static void cmd_setkeys(const char* args);
static void cmd_reboot(const char* args __attribute__((unused)));
static void cmd_memory(const char* args);
static void cmd_mkfs_rfss(const char* args);
static void cmd_mount(const char* args);
static void cmd_umount(const char* args);
static void cmd_ls(const char* args);
static void cmd_cd(const char* args);
static void cmd_mkdir(const char* args);
static void cmd_rmdir(const char* args);
static void cmd_touch(const char* args);
static void cmd_cp(const char* args);
static void cmd_mv(const char* args);
static void cmd_rm(const char* args);
static void cmd_cat(const char* args);
static void cmd_df(const char* args);
static void cmd_fsck_rfss(const char* args);
static void cmd_lsdisk(const char* args);
static void cmd_startx(const char* args);
static void cmd_forktest(const char* args);
static void cmd_exit(const char* args);
static void cmd_lsusb(const char* args);
static void cmd_rsh(const char* args);
static void cmd_credits(const char* args);

static uint32_t parse_ip(const char* str);
static int is_ip_address(const char* str);
static void cmd_ping(const char* args);

static const command_t commands[] = {
    {"help", "Display this help message", cmd_help, CMD_SAFE},
    {"vian", "Baaah", cmd_vian, CMD_SAFE},
    {"clear", "Clear the screen", cmd_clear, CMD_SAFE},
    {"info", "Display information about the system", cmd_info, CMD_SAFE},
    {"echo", "Repeats your input", cmd_echo, CMD_SAFE},
    {"setkeys", "Set keyboard layout. Use -ls to list layouts.", cmd_setkeys, CMD_SAFE},
    {"reboot", "Reboot the system.", cmd_reboot, CMD_UNSAFE},
    {"lsdisk", "List available disk devices", cmd_lsdisk, CMD_SAFE},
    {"mkfs.rfss", "Format disk with Rfss+ filesystem", cmd_mkfs_rfss, CMD_UNSAFE},
    {"mount", "Mount Rfss+ filesystem", cmd_mount, CMD_UNSAFE},
    {"umount", "Unmount Rfss+ filesystem", cmd_umount, CMD_UNSAFE},
    {"ls", "List directory contents", cmd_ls, CMD_SAFE},
    {"cd", "Change directory", cmd_cd, CMD_SAFE},
    {"mkdir", "Create directory", cmd_mkdir, CMD_SAFE},
    {"rmdir", "Remove directory", cmd_rmdir, CMD_SAFE},
    {"touch", "Create empty file", cmd_touch, CMD_SAFE},
    {"cp", "Copy files", cmd_cp, CMD_SAFE},
    {"mv", "Move files", cmd_mv, CMD_MAINTENANCE},
    {"rm", "Remove files", cmd_rm, CMD_SAFE},
    {"cat", "Display file contents", cmd_cat, CMD_SAFE},
    {"df", "Show filesystem usage", cmd_df, CMD_SAFE},
    {"fsck.rfss", "Check filesystem consistency", cmd_fsck_rfss, CMD_MAINTENANCE},
    {"startx", "Start the desktop environment", cmd_startx, CMD_SAFE},
    {"forktest", "Test fork syscall", cmd_forktest, CMD_SAFE},
    {"exit", "Exit the application", cmd_exit, CMD_SAFE},
    {"edit", "Edit a file by appending content", cmd_edit, CMD_SAFE},
    {"lsusb", "List USB devices", cmd_lsusb, CMD_SAFE},
    {"rsh", "Execute .rsh script", cmd_rsh, CMD_SAFE},
    {"ping", "Send ICMP echo request to hostname or IP address", cmd_ping, CMD_UNSAFE},
    {"credit", "RubyOS author(s).", cmd_credits, CMD_SAFE},
    {NULL, NULL, NULL, 0}
};

static input_callback_t input_callback = NULL;
static char history[10][256];
static int history_count = 0;
static int history_index = -1;
static char input_buffer[256];
static int input_pos = 0;
static int input_start_x = 0;

void shell_input_char(char c);
void shell_special_key(uint8_t scancode);
void shell_redisplay_input(void);

void shell_welcome(void)
{
    // Display color palette at the top
    uint32_t colors[] = {
        0x00000000, // Black
        0x00FFFFFF, // White
        0x00FF0000, // Red
        0x0000FF00, // Green
        0x000000FF, // Blue
        0x00FFFF00, // Yellow
        0x00FF00FF, // Magenta
        0x0000FFFF, // Cyan
        0x00808080, // Gray
        0x00C0C0C0, // Light Gray
        0x00800000, // Dark Red
        0x00008000, // Dark Green
        0x00000080, // Dark Blue
        0x00808000, // Olive
        0x00800080, // Purple
        0x00008080  // Teal
    };

    console_set_color(0x00FF0000, 0x00000000);
    printf("Welcome to RubyShell V%s\n", SHELL_VERSION);
    console_set_color(0x00FFFFFF, 0x00000000);
    printf("RubyOS - A hobbist operating system built with C\n");
    printf("Type \"help\" for a list of available commands.\n");
    printf("Type \"info\" for system information.\n");

    int num_colors = sizeof(colors) / sizeof(colors[0]);
    for (int i = 0; i < num_colors; i++) {
        console_draw_rect(2, 1, colors[i], 0);
    }
    console_putchar('\n');
    printf("\n");
}

void init_shell(void) {
    log(LOG_OK, "RubyShell initialized");
    console_clear();
    shell_welcome();
    shell_display_prompt();
    keyboard_set_char_callback(shell_input_char);
    keyboard_set_special_callback(shell_special_key);
}

void shell_set_input_callback(input_callback_t callback) {
    input_callback = callback;
}

void shell_handle_input(const char* input) {
    if (!input || !*input) {
        printf("\n");
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
            printf("\n");
            shell_display_prompt();
            return;
        }
    }

    printf("Unknown command: %s\n", cmd_name);
    printf("\n");
    shell_display_prompt();
}

void shell_execute_command(const char* input) {
    if (!input || !*input) {
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
            return;
        }
    }

    printf("Unknown command: %s\n", cmd_name);
}

void shell_display_prompt(void) {
    if (is_editmode()) return;
    printf("[ ");
    console_set_color(0x00FF0000, 0x00000000);
    printf("RubyOS");
    console_set_color(0x00FFFFFF, 0x00000000);
    printf(" ]");

    rfss_fs_t* fs = rfss_get_mounted_fs();
    if (fs && fs->mounted) {
        printf(" [%s]", fs->current_path);
    } else {
        printf(" [~]");
    }
    printf(" -$ ");
    input_start_x = console_get_cursor_x();
}

static void cmd_help(const char* args __attribute__((unused))) {
    printf("========== ");
    console_set_color(0x00B01010, 0x00F5F5F5);
    printf("Available commands (%u commands):", (sizeof(commands) / sizeof(commands[0])));
    console_set_color(0x00FFFFFF, 0x00000000);
    printf(" ==========\n");
    for (const command_t* cmd = commands; cmd->name != NULL; cmd++) {
        if (cmd->safety == CMD_UNSAFE) {
            console_set_color(0xFFFF0000, 0x00000000);
            printf("  [U] ");
        } else if (cmd->safety == CMD_MAINTENANCE) {
            console_set_color(0xFFFFFF00, 0x00000000);
            printf("  [M] ");
        } else {
            console_set_color(0xFF00FF00, 0x00000000);
            printf("  [S] ");
        }
        console_set_color(0x00FFFFFF, 0x00000000);
        printf("%s -- %s\n", cmd->name, cmd->description);
    }
    printf(" ==========\n");

    printf("\nCommand safety:\n");
    console_draw_rect(2, 1, 0xFF00FF00, 0); printf(" Safe     ");
    console_draw_rect(2, 1, 0xFFFF0000, 0); printf(" Unsafe   ");
    console_draw_rect(2, 1, 0xFFFFFF00, 0); printf(" Maintenance\n");
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
        {"ram_mb", FIELD_INT, &info.total_ram}
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


static void cmd_reboot(const char* args __attribute__((unused)))
{
    log(LOG_SYSTEM, "Rebooting..");
    outb(0x64, 0xFE);
}

static void cmd_mkfs_rfss(const char* args) {
    if (!args || !*args) {
        printf("Usage: mkfs.rfss <device> [label]\n");
        return;
    }

    char* device_str = kmalloc(32);
    if (!device_str) {
        printf("Memory allocation failed\n");
        return;
    }
    char* label = kmalloc(16);
    if (!label) {
        kfree(device_str);
        printf("Memory allocation failed\n");
        return;
    }
    memset(label, 0, 16);
    int device_id = 0;

    const char* p = args;
    int i = 0;
    while (*p && *p != ' ' && i < 31) {
        device_str[i++] = *p++;
    }
    device_str[i] = '\0';
    if (*p == ' ') p++;
    i = 0;
    while (*p && *p != ' ' && i < 15) {
        label[i++] = *p++;
    }
    label[i] = '\0';

    if (strcmp(device_str, "hda") == 0) {
        device_id = 0;
    } else {
        printf("Unsupported device: %s\n", device_str);
        kfree(device_str);
        kfree(label);
        return;
    }

    if (!ata_drive_exists(device_id)) {
        printf("Device %s not found\n", device_str);
        kfree(device_str);
        kfree(label);
        return;
    }

    printf("Formatting %s with Rfss+ filesystem...\n", device_str);
    if (rfss_format(device_id, strlen(label) > 0 ? label : NULL) == 0) {
        printf("Filesystem created successfully\n");
    } else {
        printf("Failed to create filesystem\n");
    }
    kfree(device_str);
    kfree(label);
}

static void cmd_mount(const char* args) {
    if (!args || !*args) {
        printf("Usage: mount <device>\n");
        return;
    }

    char* device_str = kmalloc(32);
    if (!device_str) {
        printf("Memory allocation failed\n");
        return;
    }
    int device_id = 0;

    const char* p = args;
    int i = 0;
    while (*p && *p != ' ' && i < 31) {
        device_str[i++] = *p++;
    }
    device_str[i] = '\0';

    if (strcmp(device_str, "hda") == 0) {
        device_id = 0;
    } else {
        printf("Unsupported device: %s\n", device_str);
        kfree(device_str);
        return;
    }

    if (!ata_drive_exists(device_id)) {
        printf("Device %s not found\n", device_str);
        kfree(device_str);
        return;
    }

    static rfss_fs_t filesystem;
    if (rfss_mount(device_id, &filesystem) == 0) {
        printf("Filesystem mounted successfully\n");
        rfss_journal_replay(&filesystem);
    } else {
        printf("Failed to mount filesystem\n");
    }
    kfree(device_str);
}

static void cmd_umount(const char* args) {
    rfss_fs_t* fs = rfss_get_mounted_fs();
    if (!fs || !fs->mounted) {
        printf("No filesystem mounted\n");
        return;
    }
    
    if (rfss_unmount(fs) == 0) {
        printf("Filesystem unmounted successfully\n");
    } else {
        printf("Failed to unmount filesystem\n");
    }
}

static void cmd_ls(const char* args) {
    rfss_fs_t* fs = rfss_get_mounted_fs();
    if (!fs || !fs->mounted) {
        printf("No filesystem mounted\n");
        return;
    }

    if (args && *args && strchr(args, '/') == NULL && !fs_isvalidname(args)) {
        printf("Invalid path: no spaces or special characters allowed\n");
        return;
    }

    rfss_dir_entry_t* entries;
    int count;

    if (rfss_list_directory(fs, args, &entries, &count) != 0) {
        printf("Failed to list directory\n");
        return;
    }
    
    if (count == 0) {
        printf("\n");
        return;
    }
    
    for (int i = 0; i < count; i++) {
        if (entries[i].name_len == 1 && entries[i].name[0] == '.') {
            continue;
        }
        if (entries[i].name_len == 2 && entries[i].name[0] == '.' && entries[i].name[1] == '.') {
            continue;
        }
        
        if (entries[i].name_len > 0 && entries[i].name_len < RFSS_MAX_FILENAME) {
            char filename[RFSS_MAX_FILENAME + 1];
            memset(filename, 0, sizeof(filename));
            memcpy(filename, entries[i].name, entries[i].name_len);
            filename[entries[i].name_len] = '\0';
            
            int j;
            for (j = 0; j < entries[i].name_len; j++) {
                if (filename[j] < 32 || filename[j] > 126) {
                    filename[j] = '?';
                }
            }
            
            printf("%s ", filename);
        }
    }
    printf("\n");
    if (entries) kfree(entries);
    
}

static void cmd_cd(const char* args) {
    rfss_fs_t* fs = rfss_get_mounted_fs();
    if (!fs || !fs->mounted) {
        printf("No filesystem mounted\n");
        return;
    }

    if (!args || !*args) {
        return;
    }

    if (strchr(args, '/') == NULL && strcmp(args, "..") != 0 && !fs_isvalidname(args)) {
        printf("Invalid directory name: no spaces or special characters allowed\n");
        return;
    }

    char new_path[256];
    if (strcmp(args, "..") == 0) {
        if (strcmp(fs->current_path, "/") == 0) {
            return; // already root
        } else {
            strcpy(new_path, fs->current_path);
            char* last = strrchr(new_path, '/');
            if (last == new_path) {
                strcpy(new_path, "/");
            } else {
                *last = '\0';
            }
        }
    } else if (args[0] == '/') {
        strcpy(new_path, args);
    } else {
        strcpy(new_path, fs->current_path);
        if (strcmp(new_path, "/") != 0) {
            strncat(new_path, "/", sizeof(new_path) - strlen(new_path) - 1);
        }
        strncat(new_path, args, sizeof(new_path) - strlen(new_path) - 1);
    }

    if (rfss_change_directory(fs, new_path) != 0) {
        printf("cd: %s: No such directory\n", args);
    } else {
        strcpy(fs->current_path, new_path);
    }
}

static void cmd_mkdir(const char* args) {
    rfss_fs_t* fs = rfss_get_mounted_fs();
    if (!fs || !fs->mounted) {
        printf("No filesystem mounted\n");
        return;
    }

    if (!args || !*args) {
        printf("Usage: mkdir <directory>\n");
        return;
    }

    if (fs_isvalidname(args) == FS_VALID) {
        printf("Invalid directory name: no spaces or special characters allowed\n");
        return;
    }

    if(rfss_create_directory(fs, args) != 0) printf("mkdir: failed to create '%s'\n", args);
}

static void cmd_rmdir(const char* args) {
    rfss_fs_t* fs = rfss_get_mounted_fs();
    if (!fs || !fs->mounted) {
        printf("No filesystem mounted\n");
        return;
    }

    if (!args || !*args) {
        printf("Usage: rmdir <directory>\n");
        return;
    }

    if (!fs_isvalidname(args)) {
        printf("Invalid filename: no spaces or special characters allowed\n");
        return;
    }

    if (rfss_remove_directory(fs, args) != 0) printf("rmdir: failed to remove '%s'\n", args);

}

static void cmd_touch(const char* args) {
    rfss_fs_t* fs = rfss_get_mounted_fs();
    if (!fs || !fs->mounted) {
        printf("No filesystem mounted\n");
        return;
    }

    if (!args || !*args) {
        printf("Usage: touch <file>\n");
        return;
    }

    if (fs_isvalidname(args) == FS_VALID) {
        printf("Invalid filename: no spaces or special characters allowed\n");
        return;
    }

    if (rfss_create_file(fs, args, 0644) != 0) {
        printf("touch: cannot create '%s'\n", args);
    }
}

static void cmd_cp(const char* args) {
    rfss_fs_t* fs = rfss_get_mounted_fs();
    if (!fs || !fs->mounted) {
        printf("No filesystem mounted\n");
        return;
    }

    if (!args || !*args) {
        printf("Usage: cp <source> <destination>\n");
        return;
    }

    char* source = kmalloc(256);
    if (!source) {
        printf("Memory allocation failed\n");
        return;
    }
    char* dest = kmalloc(256);
    if (!dest) {
        kfree(source);
        printf("Memory allocation failed\n");
        return;
    }

    const char* p = args;
    int i = 0;
    while (*p && *p != ' ' && i < 255) {
        source[i++] = *p++;
    }
    source[i] = '\0';
    if (*p == ' ') p++;
    i = 0;
    while (*p && *p != ' ' && i < 255) {
        dest[i++] = *p++;
    }
    dest[i] = '\0';

    if (strlen(source) == 0 || strlen(dest) == 0) {
        printf("Usage: cp <source> <destination>\n");
        kfree(source);
        kfree(dest);
        return;
    }

    if (!fs_isvalidname(source)) {
        printf("Invalid source filename: no spaces or special characters allowed\n");
        kfree(source);
        kfree(dest);
        return;
    }

    if (!fs_isvalidname(dest)) {
        printf("Invalid destination filename: no spaces or special characters allowed\n");
        kfree(source);
        kfree(dest);
        return;
    }
    
    rfss_file_t src_file, dst_file;
    if (rfss_open_file(fs, source, 0, &src_file) != 0) {
        printf("cp: cannot open '%s'\n", source);
        return;
    }
    
    if (rfss_create_file(fs, dest, 0644) != 0) {
        printf("cp: cannot create '%s'\n", dest);
        rfss_close_file(&src_file);
        return;
    }
    
    if (rfss_open_file(fs, dest, 1, &dst_file) != 0) {
        printf("cp: cannot open '%s' for writing\n", dest);
        rfss_close_file(&src_file);
        return;
    }
    
    uint8_t buffer[1024];
    int bytes_read;
    while ((bytes_read = rfss_read_file(&src_file, buffer, sizeof(buffer))) > 0) {
        if (rfss_write_file(&dst_file, buffer, bytes_read) != bytes_read) {
            printf("cp: write error\n");
            break;
        }
    }
    
    rfss_close_file(&src_file);
    rfss_close_file(&dst_file);
    kfree(source);
    kfree(dest);
}

static void cmd_mv(const char* args) {
    if (!args || !*args) {
        printf("Usage: mv <source> <destination>\n");
        return;
    }

    char* source = kmalloc(256);
    if (!source) {
        printf("Memory allocation failed\n");
        return;
    }
    char* dest = kmalloc(256);
    if (!dest) {
        kfree(source);
        printf("Memory allocation failed\n");
        return;
    }

    const char* p = args;
    int i = 0;
    while (*p && *p != ' ' && i < 255) {
        source[i++] = *p++;
    }
    source[i] = '\0';
    if (*p == ' ') p++;
    i = 0;
    while (*p && *p != ' ' && i < 255) {
        dest[i++] = *p++;
    }
    dest[i] = '\0';

    if (strlen(source) == 0 || strlen(dest) == 0) {
        printf("Usage: mv <source> <destination>\n");
        kfree(source);
        kfree(dest);
        return;
    }

    if (!fs_isvalidname(source)) {
        printf("Invalid source filename: no spaces or special characters allowed\n");
        kfree(source);
        kfree(dest);
        return;
    }

    if (!fs_isvalidname(dest)) {
        printf("Invalid destination filename: no spaces or special characters allowed\n");
        kfree(source);
        kfree(dest);
        return;
    }

    printf("mv: not implemented yet\n");
    kfree(source);
    kfree(dest);
}

static void cmd_rm(const char* args) {
    rfss_fs_t* fs = rfss_get_mounted_fs();
    if (!fs || !fs->mounted) {
        printf("No filesystem mounted\n");
        return;
    }

    if (!args || !*args) {
        printf("Usage: rm <file>\n");
        return;
    }

    if (!fs_isvalidname(args)) {
        printf("Invalid filename: no spaces or special characters allowed\n");
        return;
    }

    if (rfss_delete_file(fs, args) != 0) {
        printf("rm: cannot remove '%s'\n", args);
    }
}

static void cmd_cat(const char* args) {
    rfss_fs_t* fs = rfss_get_mounted_fs();
    if (!fs || !fs->mounted) {
        printf("No filesystem mounted\n");
        return;
    }

    if (!args || !*args) {
        printf("Usage: cat <file>\n");
        return;
    }

    if (!fs_isvalidname(args)) {
        printf("Invalid filename: no spaces or special characters allowed\n");
        return;
    }

    rfss_file_t file;
    if (rfss_open_file(fs, args, 0, &file) != 0) {
        printf("cat: cannot open '%s'\n", args);
        return;
    }

    uint8_t buffer[1024];
    int bytes_read;
    while ((bytes_read = rfss_read_file(&file, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            putchar(buffer[i]);
        }
    }

    rfss_close_file(&file);
    putchar('\n');
}

static void cmd_df(const char* args) {
    rfss_fs_t* fs = rfss_get_mounted_fs();
    if (!fs || !fs->mounted) {
        printf("No filesystem mounted\n");
        return;
    }
    
    uint32_t total_blocks, free_blocks, total_inodes, free_inodes;
    if (rfss_get_stats(fs, &total_blocks, &free_blocks, &total_inodes, &free_inodes) != 0) {
        printf("Failed to get filesystem statistics\n");
        return;
    }
    
    uint32_t used_blocks = total_blocks - free_blocks;
    uint32_t used_inodes = total_inodes - free_inodes;
    
    printf("Filesystem statistics:\n");
    printf("Blocks: %d total, %d used, %d free (%d%% used)\n",
           total_blocks, used_blocks, free_blocks,
           total_blocks > 0 ? (used_blocks * 100) / total_blocks : 0);
    printf("Inodes: %d total, %d used, %d free (%d%% used)\n",
           total_inodes, used_inodes, free_inodes,
           total_inodes > 0 ? (used_inodes * 100) / total_inodes : 0);
    printf("Block size: %d bytes\n", RFSS_BLOCK_SIZE);
    printf("Total size: %d KB\n", (total_blocks * RFSS_BLOCK_SIZE) / 1024);
    printf("Free space: %d KB\n", (free_blocks * RFSS_BLOCK_SIZE) / 1024);
}

static void cmd_fsck_rfss(const char* args __attribute__((unused))) {
    rfss_fs_t* fs = rfss_get_mounted_fs();
    if (!fs || !fs->mounted) {
        printf("No filesystem mounted\n");
        return;
    }

    printf("Checking filesystem consistency...\n");
    if (rfss_check_filesystem(fs) == 0) {
        printf("Filesystem is clean\n");
    } else {
        printf("Filesystem has errors\n");
    }
}

void shell_input_char(char c) {
    if (is_editmode()) {
        edit_input_char(c);
        return;
    }
    if (c == '\b') {
        if (input_pos > 0) {
            input_pos--;
            putchar('\b');
            putchar(' ');
            putchar('\b');
        }
    } else if (c == '\n') {
        if (is_editmode()) {
            putchar('\n');
            input_pos = 0;
            input_buffer[0] = '\0';
            return;
        }
        input_buffer[input_pos] = '\0';
        if (input_pos > 0) {
            if (history_count < 10) {
                strcpy(history[history_count], input_buffer);
                history_count++;
            } else {
                for (int i = 0; i < 9; i++) strcpy(history[i], history[i+1]);
                strcpy(history[9], input_buffer);
            }
        }
        history_index = -1;
        putchar('\n');
        shell_handle_input(input_buffer);
        input_pos = 0;
    } else if (input_pos < 255) {
        input_buffer[input_pos++] = c;
        putchar(c);
    }
}

void shell_special_key(uint8_t scancode) {
    if (is_editmode() && scancode == 0x01) { // ESC
        // save file
        int saved = 0;
        rfss_fs_t* fs = rfss_get_mounted_fs();
        if (fs && fs->mounted) {
            rfss_file_t file;
            if (rfss_open_file(fs, current_file, 1, &file) == 0) {
                if (rfss_write_file(&file, (uint8_t*)edit_buffer, buffer_pos) == buffer_pos) {
                    saved = 1;
                }
                rfss_close_file(&file);
            }
        }
        console_clear();
        if (saved) {
            printf("File saved successfully.\n");
        } else {
            printf("Error saving file.\n");
        }
        set_editmode(0);
        shell_display_prompt();
        return;
    }
    if (is_editmode()) {
        if (scancode == 0x4B) { // left arrow
            edit_move_cursor_left();
        } else if (scancode == 0x4D) { // right arrow
            edit_move_cursor_right();
        }
        return;
    }
    if (scancode == 0x48) { // up
        if (history_count == 0) return;
        if (history_index == -1) {
            history_index = history_count - 1;
        } else if (history_index > 0) {
            history_index--;
        }
        strcpy(input_buffer, history[history_index]);
        input_pos = strlen(input_buffer);
        shell_redisplay_input();
    } else if (scancode == 0x50) { // down
        if (history_index == -1) return;
        if (history_index < history_count - 1) {
            history_index++;
            strcpy(input_buffer, history[history_index]);
            input_pos = strlen(input_buffer);
        } else {
            history_index = -1;
            input_pos = 0;
            input_buffer[0] = '\0';
        }
        shell_redisplay_input();
    }
}

void shell_redisplay_input() {
    uint32_t y = console_get_cursor_y();
    console_set_cursor(input_start_x, y);
    for (int i = 0; i < 60; i++) putchar(' ');
    console_set_cursor(input_start_x, y);
    printf("%s", input_buffer);
}

static void cmd_lsdisk(const char* args __attribute__((unused))) {
    printf("Available disk devices:\n");
    ata_list_devices();
}

static void cmd_startx(const char* args __attribute__((unused))) {
    init_desktop();
    desktop_event_loop();
    init_shell();
}

static void cmd_forktest(const char* args __attribute__((unused))) {
    int pid = fork();
    if (pid == 0) {
        printf("Child process: Hello from child!\n");
    } else {
        printf("Parent process: Forked child with PID %d\n", pid);
    }
}

static void cmd_exit(const char* args __attribute__((unused))) {
    printf("Exiting...\n");
    exit(0);
}

static void cmd_mousetest(const char* args __attribute__((unused))) {
    mouse_state_t state = get_mouse_state();
    printf("Mouse coordinates: x=%d, y=%d, buttons=%d\n", state.x, state.y, state.buttons);
}

static void cmd_lsusb(const char* args __attribute__((unused))) {
    usb_device_t* device = usb_get_device_list();
    uint32_t count = usb_get_device_count();

    if (count == 0) {
        printf("No USB devices found\n");
        return;
    }

    printf("USB devices (%d):\n", count);
    while (device) {
        printf("  Address: %d, Vendor: 0x%04x, Product: 0x%04x, Class: 0x%02x",
               device->address, device->vendor_id, device->product_id, device->device_class);
        if (device->is_keyboard) {
            printf(" (Keyboard)");
        }
        if (device->is_mouse) {
            printf(" (Mouse)");
        }
        printf("\n");
        device = device->next;
    }
}

static uint32_t parse_ip(const char* str) {
    uint32_t ip = 0;
    int octet = 0;
    int shift = 24;
    const char* p = str;

    while (*p && shift >= 0) {
        if (*p == '.') {
            ip |= (octet << shift);
            octet = 0;
            shift -= 8;
        } else if (*p >= '0' && *p <= '9') {
            octet = octet * 10 + (*p - '0');
        } else {
            return 0; // Invalid
        }
        p++;
    }
    ip |= (octet << shift);
    return htonl(ip);
}

static int is_ip_address(const char* str) {
    int dots = 0;
    while (*str) {
        if (*str == '.') {
            dots++;
        } else if (*str < '0' || *str > '9') {
            return 0;
        }
        str++;
    }
    return dots == 3;
}

static void cmd_ping(const char* args) {
    if (!args || !*args) {
        printf("Usage: ping <hostname|ip>\n");
        return;
    }

    if (!rtl8139_is_ready()) {
        printf("Network card not ready\n");
        return;
    }

    uint32_t target_ip;
    if (is_ip_address(args)) {
        target_ip = parse_ip(args);
        if (target_ip == 0) {
            printf("Invalid IP address\n");
            return;
        }
        printf("Pinging %s...\n", args);
    } else {
        // Resolve hostname
        uint32_t dns_server = htonl(0x08080808); // 8.8.8.8
        target_ip = dns_resolve(args, dns_server);
        if (target_ip == 0) {
            printf("Failed to resolve hostname: %s\n", args);
            return;
        }
        printf("Pinging %s (%d.%d.%d.%d)...\n", args,
               (ntohl(target_ip) >> 24) & 0xFF,
               (ntohl(target_ip) >> 16) & 0xFF,
               (ntohl(target_ip) >> 8) & 0xFF,
               ntohl(target_ip) & 0xFF);
    }

    uint8_t data[] = {0x41, 0x42, 0x43}; // "ABC"
    icmp_send_echo_request(target_ip, 1, 1, data, sizeof(data));
    printf("ICMP echo request sent\n");
}


static void cmd_rsh(const char* args) {
    if (!args || !*args) {
        printf("Usage: rsh <file.rsh> [args...]\n");
        return;
    }

    char* script_args[10];
    int arg_count = 0;
    char* p = (char*)args;
    while (*p && arg_count < 10) {
        while (*p == ' ') p++;
        if (!*p) break;
        script_args[arg_count++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }

    if (arg_count == 0) {
        printf("Usage: rsh <file.rsh> [args...]\n");
        return;
    }

    char* filename = script_args[0];
    size_t len = strlen(filename);
    if (len < 4 || strcmp(filename + len - 4, ".rsh") != 0) {
        printf("File must have .rsh extension\n");
        return;
    }

    execute_rsh_script(filename, script_args + 1, arg_count - 1);
}

static void cmd_credits(const char* args __attribute__((unused))) {
    printf("Made with love by DarekCoffee :)\n");
}