#include "mouse_cursor.h"
#include "../drivers/screen.h"
#include <stdint.h>

static int cursor_x = 0;
static int cursor_y = 0;

void init_cursor(void) {
    cursor_x = screen.width / 2;
    cursor_y = screen.height / 2;
}

void update_cursor(int x, int y) {
    cursor_x = x;
    cursor_y = y;
    if (cursor_x < 0) cursor_x = 0;
    if (cursor_y < 0) cursor_y = 0;
    if (cursor_x > (int)screen.width - 10) cursor_x = screen.width - 10;
    if (cursor_y > (int)screen.height - 10) cursor_y = screen.height - 10;
}

void render_cursor(void) {
    int x = cursor_x;
    int y = cursor_y;
    draw_rect(x, y, 10, 10, 0x00FFFFFF);
}

int get_cursor_x(void) {
    return cursor_x;
}

int get_cursor_y(void) {
    return cursor_y;
}