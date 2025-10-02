#include "mouse.h"
#include "../usb_device.h"
#include "../usb_transfer.h"
#include <../drivers/mouse/mouse.h>
#include <logger.h>

#define USB_ENDPOINT_IN 0x80

static int8_t last_x = 0;
static int8_t last_y = 0;
static uint8_t last_buttons = 0;

void usb_mouse_init(void) {
    log(LOG_SYSTEM, "USB Mouse initialized");
}

void usb_mouse_handler(uint8_t endpoint, usb_mouse_report_t* report) {
    if (!report) return;

    int8_t dx = report->x;
    int8_t dy = report->y;
    uint8_t buttons = report->buttons;

    if (dx != last_x || dy != last_y || buttons != last_buttons) {
        mouse_update(dx, dy, buttons);
        last_x = dx;
        last_y = dy;
        last_buttons = buttons;
    }
}

uint8_t usb_mouse_probe(usb_device_t* device) {
    if (!device) return 0;

    if (device->device_class == USB_CLASS_HID &&
        device->device_subclass == USB_SUBCLASS_BOOT &&
        device->device_protocol == USB_PROTOCOL_MOUSE) {

        usb_mouse_init();
        device->is_mouse = 1;
        return 1;
    }
    return 0;
}