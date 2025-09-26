#include "mouse_cursor.h"
#include "../drivers/screen.h"

static int cursor_x = 0;
static int cursor_y = 0;

void init_cursor(void) {
    cursor_x = 400;
    cursor_y = 300;
}

void update_cursor(int x, int y) {
    cursor_x = x;
    cursor_y = y;
    if (cursor_x < 0) cursor_x = 0;
    if (cursor_y < 0) cursor_y = 0;
    if (cursor_x > 800) cursor_x = 800;
    if (cursor_y > 600) cursor_y = 600;
}

void render_cursor(void) {
    int x = cursor_x;
    int y = cursor_y;
    set_pixel(x, y, 0x00FFFFFF);
    set_pixel(x+1, y, 0x00FFFFFF);
    set_pixel(x, y+1, 0x00FFFFFF);
    set_pixel(x+1, y+1, 0x00FFFFFF);
    set_pixel(x+2, y+1, 0x00FFFFFF);
    set_pixel(x+1, y+2, 0x00FFFFFF);
}

int get_cursor_x(void) {
    return cursor_x;
}

int get_cursor_y(void) {
    return cursor_y;
}