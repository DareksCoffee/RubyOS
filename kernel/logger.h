#ifndef __LOGGER_H
#define __LOGGER_H

#include <stdint.h>

#define LOG_LEVELS \
    X(LOG_OK,      "OK",      0x0000FF00) \
    X(LOG_FAIL,    "FAIL",    0x00FF0000) \
    X(LOG_LOG,     "LOG",     0x00FFFFFF) \
    X(LOG_WARNING, "WARNING", 0x00FFFF00) \
    X(LOG_SYSTEM,  "SYSTEM",  0x0000FFFF) \
    X(LOG_DEBUG,   "DEBUG",   0x00855454) \
    X(LOG_ERROR,   "ERROR",   0x00FF0000)

typedef enum {
#define X(name, str, color) name,
    LOG_LEVELS
#undef X
    LOG_COUNT
} LogLevel;

void log(LogLevel level, const char* format, ...);

#endif // LOGGER_H
