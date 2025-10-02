#include <stdio.h>
#include <string.h>
#include <info.h>
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
#include "../drivers/console.h"
#include "edit.h"


static int editmode = 0;
char edit_buffer[4096];
int buffer_pos = 0;
int cursor_pos = 0;
char current_file[256];


void cmd_edit(const char* args) {
    if (!args || !*args) {
        printf("Usage: edit <file>\n");
        return;
    }
    strcpy(current_file, args);
    rfss_fs_t* fs = rfss_get_mounted_fs();
    if (!fs || !fs->mounted) {
        printf("No filesystem mounted\n");
        return;
    }
    rfss_file_t file;
    if (rfss_open_file(fs, args, 0, &file) != 0) {
        printf("File not found: %s\n", args);
        return;
    }
    buffer_pos = 0;
    uint8_t buf[1024];
    int bytes;
    while ((bytes = rfss_read_file(&file, buf, sizeof(buf))) > 0) {
        for (int i = 0; i < bytes && buffer_pos < sizeof(edit_buffer) - 1; i++) {
            edit_buffer[buffer_pos++] = buf[i];
        }
    }
    edit_buffer[buffer_pos] = '\0';
    rfss_close_file(&file);
    cursor_pos = buffer_pos;
    redraw_edit();
    set_editmode(1);
}

int is_editmode()
{
    return editmode;
}

void set_editmode(int mode)
{
    editmode = mode;
}

void redraw_edit() {
    console_clear();

    // Display header with file name in white background
    console_set_color(0x000000, 0xFFFFFF); // black text on white bg
    console_printf("Editing: %s\n", current_file);
    console_set_color(0xFFFFFF, 0x000000); // reset to white text on black bg

    // Count lines and characters
    int lines = 1;
    for (int i = 0; i < buffer_pos; i++) {
        if (edit_buffer[i] == '\n') lines++;
    }
    int chars = buffer_pos;

    // Display lines and characters
    console_printf("Lines: %d, Characters: %d\n\n", lines, chars);

    // Print the file content
    printf("%s", edit_buffer);
}

void edit_move_cursor_left() {
    if (cursor_pos > 0) cursor_pos--;
    redraw_edit();
}

void edit_move_cursor_right() {
    if (cursor_pos < buffer_pos) cursor_pos++;
    redraw_edit();
}

void edit_input_char(char c)
{
    if (c == '\b') {
        if (cursor_pos > 0) {
            // delete at cursor_pos - 1
            for (int i = cursor_pos - 1; i < buffer_pos - 1; i++) {
                edit_buffer[i] = edit_buffer[i + 1];
            }
            buffer_pos--;
            cursor_pos--;
            edit_buffer[buffer_pos] = '\0';
        }
    } else if (c == '\n') {
        if (buffer_pos < sizeof(edit_buffer) - 1) {
            // insert \n at cursor_pos
            for (int i = buffer_pos; i > cursor_pos; i--) {
                edit_buffer[i] = edit_buffer[i - 1];
            }
            edit_buffer[cursor_pos] = '\n';
            buffer_pos++;
            cursor_pos++;
            edit_buffer[buffer_pos] = '\0';
        }
    } else {
        if (buffer_pos < sizeof(edit_buffer) - 1) {
            // insert c at cursor_pos
            for (int i = buffer_pos; i > cursor_pos; i--) {
                edit_buffer[i] = edit_buffer[i - 1];
            }
            edit_buffer[cursor_pos] = c;
            buffer_pos++;
            cursor_pos++;
            edit_buffer[buffer_pos] = '\0';
        }
    }
    redraw_edit();
}
