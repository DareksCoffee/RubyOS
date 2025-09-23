#include "theme.h"

static theme_t current_theme;

void init_theme(void) {
    current_theme.os_name = "RubyOS";
    current_theme.bracket_color = 0x00FFFFFF; // White
    current_theme.os_name_color = 0x00FF0000; // Red
    current_theme.prompt_symbol_color = 0x00FFFFFF; // White
    current_theme.default_text_color = 0x00FFFFFF; // White
    current_theme.default_bg_color = 0x00000000; // Black
}

theme_t* get_theme(void) {
    return &current_theme;
}
