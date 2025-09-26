#ifndef USB_COMMANDS_H
#define USB_COMMANDS_H

#define USB_REQ_GET_STATUS              0x00
#define USB_REQ_CLEAR_FEATURE          0x01
#define USB_REQ_SET_FEATURE            0x03
#define USB_REQ_SET_ADDRESS            0x05
#define USB_REQ_GET_DESCRIPTOR         0x06
#define USB_REQ_SET_DESCRIPTOR         0x07
#define USB_REQ_GET_CONFIGURATION      0x08
#define USB_REQ_SET_CONFIGURATION      0x09
#define USB_REQ_GET_INTERFACE          0x0A
#define USB_REQ_SET_INTERFACE          0x0B
#define USB_REQ_SYNCH_FRAME            0x0C

#define USB_DESC_TYPE_DEVICE           0x01
#define USB_DESC_TYPE_CONFIG           0x02
#define USB_DESC_TYPE_STRING           0x03
#define USB_DESC_TYPE_INTERFACE        0x04
#define USB_DESC_TYPE_ENDPOINT         0x05

#endif
