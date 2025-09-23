#ifndef SCANCODE_H
#define SCANCODE_H

#define KEY_RELEASE 0x80

typedef enum {
    KBD_LAYOUT_US,
    KBD_LAYOUT_FR,
    KBD_LAYOUT_DE,
    KBD_LAYOUT_COUNT
} keyboard_layout_t;

typedef struct {
    const char* name;
    const char* map;
    const char* shift_map;
} scancode_map_t;

static const char scancode_us[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char scancode_us_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};


static const char scancode_fr[] = {
    0, 0, '&', 233, '\"', '\'', '(', '-', 232, '_', 231, 224, ')', '=', '\b',
    '\t', 'a', 'z', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '^', '$', '\n',
    0, 'q', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 249, '*',
    0, '<', 'w', 'x', 'c', 'v', 'b', 'n', ',', ';', ':', '!', 0,
    '*', 0, ' '
};

static const char scancode_fr_shift[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 223, '+', '\b',
    '\t', 'A', 'Z', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '¨', '£', '\n',
    0, 'Q', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'M', '%', '`',
    0, '>', 'W', 'X', 'C', 'V', 'B', 'N', '?', '.', '/', '§', 0,
    '*', 0, ' '
};

static const char scancode_de[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'ß', '\'', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'z', 'u', 'i', 'o', 'p', 'ü', '+', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'ö', 'ä', '#',
    0, '<', 'y', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '-', 0,
    '*', 0, ' '
};

static const char scancode_de_shift[] = {
    0, 0, '!', '"', '§', '$', '%', '&', '/', '(', ')', '=', '?', '`', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Z', 'U', 'I', 'O', 'P', 'Ü', '*', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'Ö', 'Ä', '\'',
    0, '>', 'Y', 'X', 'C', 'V', 'B', 'N', 'M', ';', ':', '_', 0,
    '*', 0, ' '
};
static const scancode_map_t keyboard_layouts[KBD_LAYOUT_COUNT] = {
    [KBD_LAYOUT_US] = {"us", scancode_us, scancode_us_shift},
    [KBD_LAYOUT_FR] = {"fr", scancode_fr, scancode_fr_shift}, 
    [KBD_LAYOUT_DE] = {"de", scancode_de, scancode_de_shift}
};

#endif

