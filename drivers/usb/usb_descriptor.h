#ifndef USB_DESCRIPTOR_H
#define USB_DESCRIPTOR_H

#include <stdint.h>

typedef struct {
    uint8_t length;
    uint8_t descriptor_type;
    uint16_t bcd_usb;
    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t device_protocol;
    uint8_t max_packet_size;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t bcd_device;
    uint8_t manufacturer_index;
    uint8_t product_index;
    uint8_t serial_index;
    uint8_t num_configurations;
} __attribute__((packed)) usb_device_descriptor_t;

typedef struct {
    uint8_t length;
    uint8_t descriptor_type;
    uint16_t total_length;
    uint8_t num_interfaces;
    uint8_t configuration_value;
    uint8_t configuration_index;
    uint8_t attributes;
    uint8_t max_power;
} __attribute__((packed)) usb_config_descriptor_t;

typedef struct {
    uint8_t length;
    uint8_t descriptor_type;
    uint8_t interface_number;
    uint8_t alternate_setting;
    uint8_t num_endpoints;
    uint8_t interface_class;
    uint8_t interface_subclass;
    uint8_t interface_protocol;
    uint8_t interface_string;
} __attribute__((packed)) usb_interface_descriptor_t;

uint8_t usb_get_device_descriptor(uint8_t device_addr, usb_device_descriptor_t* desc);
uint8_t usb_get_config_descriptor(uint8_t device_addr, uint8_t config_index, usb_config_descriptor_t* desc);
uint8_t usb_get_interface_descriptor(uint8_t device_addr, uint8_t interface_index, usb_interface_descriptor_t* desc);

#endif
