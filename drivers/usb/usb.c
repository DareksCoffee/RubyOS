#include "usb.h"
#include "usb_device.h"
#include <../cpu/ports.h>
#include <logger.h>

static usb_controller_t usb_controller;

void init_usb(void) {
    usb_controller.version = 0x0200;
    usb_controller.base_address = 0x1000;  // Changed base address
    usb_controller.irq = 11;
    usb_controller.num_ports = 8;  // Increased number of ports
    usb_controller.enabled = 1;

    uint16_t vendor = inw(usb_controller.base_address);
    if (vendor != 0xFFFF) {  // Changed condition
        uint16_t command = inw(usb_controller.base_address + 4);
        command |= 0x0004;  // Enable bus mastering
        outw(usb_controller.base_address + 4, command);
        
        usb_init_device_list();
        usb_probe_ports();
    }
}

usb_controller_t* get_usb_controller(void) {
    return &usb_controller;
}

uint8_t usb_probe_ports(void) {
    if (!usb_controller.enabled) return 0;

    uint8_t valid_ports = 0;
    for (uint8_t port = 0; port < usb_controller.num_ports; port++) {
        uint16_t port_status = inw(usb_controller.base_address + 0x10 + (port * 4));
        if (port_status & 0x0001) {  // Port connected
            if (port_status & 0x0004) {  // Port enabled
                if (usb_enumerate_device(port)) {
                    valid_ports++;
                }
            }
        }
    }
    return valid_ports;
}