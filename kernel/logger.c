#include "logger.h"
#include "console.h"
#include <stdarg.h>

static const char* level_strings[LOG_COUNT] = {
#define X(name, str, color) str,
    LOG_LEVELS
#undef X
};

static const uint32_t level_colors[LOG_COUNT] = {
#define X(name, str, color) color,
    LOG_LEVELS
#undef X
};

void log(LogLevel level, const char* format, ...) {
    const char* level_str = "UNKNOWN";
    uint32_t color = 0x00FF00FF;

    if (level >= 0 && level < LOG_COUNT) {
        level_str = level_strings[level];
        color = level_colors[level];
    }

    console_set_color(0x00FFFFFF, 0x00000000);
    console_printf("[  ");

    console_set_color(color, 0x00000000);
    console_printf("%s", level_str);

    console_set_color(0x00FFFFFF, 0x00000000);
    console_printf("  ] ");

    va_list args;
    va_start(args, format);
    console_vprintf(format, args);
    va_end(args);

    console_printf("\n");
}
