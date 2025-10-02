#include "fs_util.h"

int fs_isvalidname(const char* name) {
    if (!name || !*name) return 0;
    for (const char* p = name; *p; p++) {
        if (*p == ' ' || ((*p < '0' || (*p > '9' && *p < 'A') || (*p > 'Z' && *p < 'a') || *p > 'z') && *p != '.' && *p != '_' && *p != '-')) {
            return FS_VALID;
        }
    }
    return FS_INVALID;
}