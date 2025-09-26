#ifndef USB_TRANSFER_H
#define USB_TRANSFER_H

#include <stdint.h>

#define USB_PID_SETUP 0x2D
#define USB_PID_IN    0x69
#define USB_PID_OUT   0xE1

typedef struct {
    uint8_t request_type;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t length;
} __attribute__((packed)) usb_setup_packet_t;

uint8_t usb_control_transfer(uint8_t device_addr, usb_setup_packet_t* setup, void* data);
uint8_t usb_bulk_transfer(uint8_t device_addr, uint8_t endpoint, void* data, uint16_t len);

#endif
