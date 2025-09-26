#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <stdint.h>
#include <../drivers/time/pit.h>

#define USB_MAX_DEVICES 16
#define USB_DEVICE_ATTACHED 0x01
#define USB_DEVICE_CONFIGURED 0x02

#define USB_MAX_ENDPOINTS 16

typedef struct {
    uint8_t endpoint_addr;
    uint16_t max_packet_size;
    uint8_t attributes;
} usb_endpoint_t;

typedef struct usb_device {
    uint8_t address;
    uint8_t port_number;
    uint8_t speed;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t device_protocol;
    struct usb_device* next;
    usb_endpoint_t endpoints[USB_MAX_ENDPOINTS];
    uint8_t num_endpoints;
    uint8_t is_keyboard;
    uint8_t status;
} usb_device_t;

typedef struct {
    usb_device_t* head;
    uint32_t count;
} usb_device_list_t;

void usb_init_device_list(void);
usb_device_t* usb_add_device(uint8_t port, uint8_t speed);
void usb_remove_device(usb_device_t* device);
usb_device_t* usb_get_device_list(void);
uint32_t usb_get_device_count(void);

uint8_t usb_enumerate_device(uint8_t port);
uint8_t usb_set_address(uint8_t port, uint8_t address);
uint8_t usb_set_configuration(uint8_t device_addr, uint8_t config);

#define USB_CLASS_HID 0x03
#define USB_SUBCLASS_BOOT 0x01
#define USB_PROTOCOL_KEYBOARD 0x01

#endif
