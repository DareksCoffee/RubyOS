#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H

#include <stdint.h>
#include "../usb_device.h"

#define USB_HID_KEYBOARD_CLASS    0x03
#define USB_HID_KEYBOARD_PROTOCOL 0x01

typedef struct {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
} __attribute__((packed)) usb_keyboard_report_t;

void usb_keyboard_init(void);
void usb_keyboard_handler(uint8_t endpoint, usb_keyboard_report_t* report);
uint8_t usb_keyboard_probe(struct usb_device* device);

#endif
