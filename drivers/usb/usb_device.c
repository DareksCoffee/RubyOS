#include "usb_device.h"
#include "usb_descriptor.h"
#include "usb_transfer.h"
#include <logger.h>
#include <../drivers/time/pit.h>
#include <../drivers/usb/hid/keyboard.h>
#include <../drivers/usb/hid/mouse.h>
#include <string.h>

static usb_device_list_t device_list;

void usb_init_device_list(void) {
    device_list.head = 0;
    device_list.count = 0;
}

usb_device_t* usb_add_device(uint8_t port, uint8_t speed) {
    static uint32_t last_enum_time = 0;
    uint32_t current_time = pit_ticks();
    
    if (current_time - last_enum_time < 100) {
        return 0;
    }
    last_enum_time = current_time;

    // Static array for device storage instead of fixed address
    static usb_device_t devices[16];
    static uint8_t next_device = 0;
    
    if (next_device >= 16) return 0;
    
    usb_device_t* device = &devices[next_device++];
    memset(device, 0, sizeof(usb_device_t));
    
    device->port_number = port;
    device->speed = speed;
    device->vendor_id = 0x046d;  // Example: Logitech
    device->product_id = 0xc31c;  // Example: Keyboard
    device->device_class = USB_HID_KEYBOARD_CLASS;
    device->num_endpoints = 1;
    device->next = device_list.head;
    device_list.head = device;
    device_list.count++;
    
    log(LOG_OK, "USB: Device added on port %d", port);
    return device;
}

void usb_remove_device(usb_device_t* device) {
    if (!device) return;
    
    if (device == device_list.head) {
        device_list.head = device->next;
        device_list.count--;
        return;
    }
    
    usb_device_t* current = device_list.head;
    while (current && current->next != device) {
        current = current->next;
    }
    
    if (current) {
        current->next = device->next;
        device_list.count--;
    }
}

usb_device_t* usb_get_device_list(void) {
    return device_list.head;
}

uint32_t usb_get_device_count(void) {
    return device_list.count;
}

uint8_t usb_enumerate_device(uint8_t port) {
    usb_device_t* device = usb_add_device(port, 0);
    if (!device) return 0;

    usb_device_descriptor_t desc;
    if (!usb_get_device_descriptor(0, &desc)) {
        usb_remove_device(device);
        return 0;
    }

    device->vendor_id = desc.vendor_id;
    device->product_id = desc.product_id;
    device->device_class = desc.device_class;

    if (device->device_class == USB_CLASS_HID) {
        usb_interface_descriptor_t iface;
        if (usb_get_interface_descriptor(device->address, 0, &iface)) {
            if (iface.interface_subclass == USB_SUBCLASS_BOOT &&
                iface.interface_protocol == USB_PROTOCOL_KEYBOARD) {
                device->is_keyboard = 1;
            } else if (iface.interface_subclass == USB_SUBCLASS_BOOT &&
                       iface.interface_protocol == USB_PROTOCOL_MOUSE) {
                device->is_mouse = 1;
            }
        }
    }
    return device->address;
}

uint8_t usb_set_address(uint8_t port __attribute__((unused)), uint8_t address) {
    usb_setup_packet_t setup = {
        .request_type = 0x00,
        .request = 0x05,
        .value = address,
        .index = 0,
        .length = 0
    };
    return usb_control_transfer(0, &setup, 0);
}

uint8_t usb_set_configuration(uint8_t device_addr, uint8_t config) {
    usb_setup_packet_t setup = {
        .request_type = 0x00,
        .request = 0x09,
        .value = config,
        .index = 0,
        .length = 0
    };
    return usb_control_transfer(device_addr, &setup, 0);
}