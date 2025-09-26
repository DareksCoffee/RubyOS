#include "keyboard.h"
#include "../usb_device.h"
#include "../usb_transfer.h"
#include <../drivers/keyboard/keyboard.h>
#include <logger.h>

#define USB_ENDPOINT_IN 0x80

static uint8_t last_keys[6] = {0};

void usb_keyboard_init(void) {
    log(LOG_SYSTEM, "USB Keyboard initialized");
}

static uint8_t translate_key(uint8_t usb_key) {
    static const uint8_t usb_to_scancode[] = {
        0x00, 0x29, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27, 0x2D, 0x2E, 0x2A, 0x2B
    };
    
    if (usb_key < sizeof(usb_to_scancode))
        return usb_to_scancode[usb_key];
    return 0;
}

void usb_keyboard_handler(uint8_t endpoint, usb_keyboard_report_t* report) {
    if (!report) return;

    for (int i = 0; i < 6; i++) {
        if (report->keys[i] != last_keys[i] && report->keys[i] != 0) {
            uint8_t scancode = translate_key(report->keys[i]);
            if (scancode)
                keyboard_input_char(scancode);
        }
        last_keys[i] = report->keys[i];
    }
}

uint8_t usb_keyboard_probe(usb_device_t* device) {
    if (!device) return 0;
    
    if (device->device_class == USB_CLASS_HID &&
        device->device_subclass == USB_SUBCLASS_BOOT &&
        device->device_protocol == USB_PROTOCOL_KEYBOARD) {
        
        usb_keyboard_init();
        device->is_keyboard = 1;
        return 1;
    }
    return 0;
}
