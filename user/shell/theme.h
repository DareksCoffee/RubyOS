#ifndef THEME_H
#define THEME_H

#include <stdint.h>

typedef struct {
    const char* os_name;
    uint32_t bracket_color;
    uint32_t os_name_color;
    uint32_t prompt_symbol_color;
    uint32_t default_text_color;
    uint32_t default_bg_color;
} theme_t;

void init_theme(void);
theme_t* get_theme(void);

#endif // THEME_H
