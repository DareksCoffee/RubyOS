#ifndef __RTL8139_H
#define __RTL8139_H


#include <stdio.h>
#include <stddef.h>
#include <../drivers/net/netdev.h>
#include <../cpu/ports.h>


#define RTL8139_IO_BASE         0xC000
#define RTL8139_REG_IDR0        0x00
#define RTL8139_REG_TCR         0x40
#define RTL8139_REG_RCR         0x44
#define RTL8139_REG_CR          0x37

#define CR_RX_ENABLE            0x08
#define CR_TX_ENABLE            0x04

#define RTL_NO                  0
#define RTL_YES                 1

void rtl8139_setup();
void rtl8139_init(netdev_t* dev);
void rtl8139_send(netdev_t *dev, uint8_t *frame, size_t len);
void rtl8139_poll(netdev_t* dev);
void rtl8139_register();

uint8_t rtl8139_linkup();
int rtl8139_is_ready();
int rtl8139_receive(uint8_t *buffer, size_t len);
netdev_t* rtl8139_get_dev();

#endif

// hi!