#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#define KEYBOARD_BUFFER_SIZE 256
#define PROMPT_SIZE 8  

void init_keyboard(void);
void keyboard_set_callback(void (*callback)(const char* input));
void keyboard_set_char_callback(void (*callback)(char c));
void keyboard_set_special_callback(void (*callback)(uint8_t scancode));
char* keyboard_get_buffer(void);
void keyboard_clear_buffer(void);
void keyboard_handle_backspace(void);
void keyboard_input_char(char c);
void keyboard_set_layout(const char* layout_name);
void keyboard_list_layouts(void);

#endif
