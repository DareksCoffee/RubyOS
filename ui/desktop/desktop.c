#include "desktop.h"
#include "../drivers/screen.h"
#include "../drivers/keyboard/keyboard.h"
#include "../drivers/mouse/mouse.h"

static desktop_element_t elements[3];
static int desktop_running = 1;

static void desktop_special_key(uint8_t scancode) {
    if (scancode == 0x01) desktop_running = 0;
    else if (scancode == 0x48) { // up
        update_cursor(get_cursor_x(), get_cursor_y() - 10);
    } else if (scancode == 0x50) { // down
        update_cursor(get_cursor_x(), get_cursor_y() + 10);
    } else if (scancode == 0x4B) { // left
        update_cursor(get_cursor_x() - 10, get_cursor_y());
    } else if (scancode == 0x4D) { // right
        update_cursor(get_cursor_x() + 10, get_cursor_y());
    }
}

static void desktop_mouse_callback(mouse_state_t state) {
    update_cursor(state.x, state.y);
}

void init_desktop(void) {
    init_graphics();
    fill_screen(0x00112233);
    elements[0] = (desktop_element_t){50, 50, 200, 150, 0x00FF0000};
    elements[1] = (desktop_element_t){300, 100, 150, 100, 0x0000FF00};
    elements[2] = (desktop_element_t){100, 250, 250, 80, 0x000000FF};
    init_cursor();
    keyboard_set_special_callback(desktop_special_key);
    mouse_set_callback(desktop_mouse_callback);
}

void render_desktop(void) {
    for (int i = 0; i < 3; i++) {
        draw_rect(elements[i].x, elements[i].y, elements[i].width, elements[i].height, elements[i].color);
    }
    render_cursor();
}

void desktop_event_loop(void) {
    while (desktop_running) {
        render_desktop();
    }
}