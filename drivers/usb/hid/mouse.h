#ifndef USB_MOUSE_H
#define USB_MOUSE_H

#include <stdint.h>
#include <../drivers/usb/usb_device.h>

typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
} usb_mouse_report_t;

void usb_mouse_init(void);
void usb_mouse_handler(uint8_t endpoint, usb_mouse_report_t* report);
uint8_t usb_mouse_probe(usb_device_t* device);

#endif