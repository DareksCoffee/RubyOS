#ifndef USB_H
#define USB_H

#include <stdint.h>
#include "usb_device.h"

typedef struct {
    uint32_t version;
    uint32_t base_address;
    uint8_t irq;
    uint8_t num_ports;
    uint8_t enabled;
} usb_controller_t;

void init_usb(void);
usb_controller_t* get_usb_controller(void);
uint8_t usb_probe_ports(void);
usb_device_t* usb_get_device_list(void);
uint32_t usb_get_device_count(void);
uint8_t usb_enumerate_device(uint8_t port);

#endif
