#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

typedef struct {
    int32_t x, y;
    uint8_t buttons;
} mouse_state_t;

void init_mouse(void);
mouse_state_t get_mouse_state(void);
void mouse_set_callback(void (*callback)(mouse_state_t state));
void mouse_update(int8_t dx, int8_t dy, uint8_t buttons);

#endif