#include "info.h"
#include "string.h"

typedef enum { FIELD_STRING, FIELD_INT } field_type_t;

typedef struct {
    const char* name;
    field_type_t type;
    void* ptr;
} field_map_t;