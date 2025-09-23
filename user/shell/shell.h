#ifndef SHELL_H
#define SHELL_H

typedef void (*command_handler_t)(const char* args);
typedef void (*input_callback_t)(const char* input);

void init_shell(void);
void shell_set_input_callback(input_callback_t callback);
void shell_handle_input(const char* input);
void shell_display_prompt(void);

#endif
