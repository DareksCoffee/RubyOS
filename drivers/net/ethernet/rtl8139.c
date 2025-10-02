#include "rtl8139.h"
#include "../arp.h"
#include "../ip.h"
#include "../icmp.h"

static netdev_t rtl8139_dev;

void rtl8139_init(netdev_t* dev)
{
    // Enable RX + TX
    outb(RTL8139_IO_BASE + RTL8139_REG_CR, CR_RX_ENABLE | CR_TX_ENABLE);

    // Read MAC into device struct
    for (int i = 0; i < 6; i++) {
        dev->mac[i] = inb(RTL8139_IO_BASE + RTL8139_REG_IDR0 + i);
    }

    // Initialize ARP
    arp_init();

    // Initialize IP
    ip_init();

    // Initialize ICMP
    icmp_init();

    // (RX/TX buffer setup TODO)
}

void rtl8139_setup()
{
    // Wire up callbacks
    rtl8139_dev.init = rtl8139_init;
    rtl8139_dev.send = rtl8139_send;
    rtl8139_dev.poll = rtl8139_poll;

    // Register once
    netdev_register(&rtl8139_dev);
}

void rtl8139_send(netdev_t *dev, uint8_t *frame, size_t len)
{
    (void)dev;
    (void)frame;
    (void)len;
    // TODO: TX implementation
}

void rtl8139_poll(netdev_t* dev)
{
    (void)dev;
    // TODO: RX implementation
    // Placeholder for ARP packet handling
    uint8_t buffer[1514];
    int len = rtl8139_receive(buffer, sizeof(buffer));
    if (len >= 42) { // Min Ethernet + ARP
        uint16_t eth_type = (buffer[12] << 8) | buffer[13];
        if (eth_type == ETH_TYPE_ARP) {
            arp_packet_t *arp = (arp_packet_t *)(buffer + 14);
            if (ntohs(arp->opcode) == ARP_REPLY) {
                arp_handle_reply(arp);
            }
        } else if (eth_type == ETH_TYPE_IP) {
            ip_receive_packet(buffer + 14, len - 14);
        }
    }
}

uint8_t rtl8139_linkup()
{
    uint8_t msr = inb(RTL8139_IO_BASE + 0x58);
    return (msr & (1 << 2)) ? RTL_YES : RTL_NO;
}

int rtl8139_is_ready()
{
    return rtl8139_linkup();
}

int rtl8139_receive(uint8_t *buffer, size_t len)
{
    (void)buffer;
    (void)len;
    return 0; // TODO: RX implementation
}

netdev_t* rtl8139_get_dev()
{
    return &rtl8139_dev;
}