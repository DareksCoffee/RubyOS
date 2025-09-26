#ifndef DESKTOP_H
#define DESKTOP_H

#include <stdint.h>
#include "mouse_cursor.h"

typedef struct {
    uint32_t x, y, width, height;
    uint32_t color;
} desktop_element_t;

void init_desktop(void);
void render_desktop(void);
void desktop_event_loop(void);

#endif