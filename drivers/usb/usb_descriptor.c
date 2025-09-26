#include "usb_descriptor.h"
#include "usb_transfer.h"
#include "usb_commands.h"

uint8_t usb_get_device_descriptor(uint8_t device_addr, usb_device_descriptor_t* desc) {
    usb_setup_packet_t setup = {
        .request_type = 0x80,
        .request = 0x06,
        .value = 0x0100,
        .index = 0x0000,
        .length = sizeof(usb_device_descriptor_t)
    };
    
    return usb_control_transfer(device_addr, &setup, desc);
}

uint8_t usb_get_config_descriptor(uint8_t device_addr, uint8_t config_index, usb_config_descriptor_t* desc) {
    usb_setup_packet_t setup = {
        .request_type = 0x80,
        .request = 0x06,
        .value = 0x0200 | config_index,
        .index = 0x0000,
        .length = sizeof(usb_config_descriptor_t)
    };
    
    return usb_control_transfer(device_addr, &setup, desc);
}

uint8_t usb_get_interface_descriptor(uint8_t device_addr, uint8_t interface_index, usb_interface_descriptor_t* desc) {
    usb_setup_packet_t setup = {
        .request_type = 0x80,
        .request = USB_REQ_GET_DESCRIPTOR,
        .value = (USB_DESC_TYPE_INTERFACE << 8) | interface_index,
        .index = 0x0000,
        .length = sizeof(usb_interface_descriptor_t)
    };
    return usb_control_transfer(device_addr, &setup, desc);
}
