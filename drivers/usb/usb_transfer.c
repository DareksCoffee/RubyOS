#include "usb_transfer.h"
#include "usb_commands.h"
#include <logger.h>

uint8_t usb_control_transfer(uint8_t device_addr, usb_setup_packet_t* setup, void* data) {
    if (!setup) return 0;

    uint8_t success = 1;
    uint8_t pid = setup->request_type & 0x80 ? USB_PID_IN : USB_PID_OUT;
    
    switch(setup->request) {
        case USB_REQ_SET_ADDRESS:
            log(LOG_SYSTEM, "USB: Setting address %d for device", setup->value);
            break;
            
        case USB_REQ_GET_DESCRIPTOR:
            log(LOG_SYSTEM, "USB: Getting descriptor type %d", setup->value >> 8);
            break;
            
        case USB_REQ_SET_CONFIGURATION:
            log(LOG_SYSTEM, "USB: Setting configuration %d", setup->value);
            break;
            
        default:
            log(LOG_WARNING, "USB: Unknown request %d", setup->request);
            success = 0;
            break;
    }

    return success;
}

uint8_t usb_bulk_transfer(uint8_t device_addr, uint8_t endpoint, void* data, uint16_t len) {
    uint8_t success = 1;
    log(LOG_OK, "USB: Bulk transfer to device %d, endpoint %d, length %d", 
        device_addr, endpoint, len);
    return success;
}
