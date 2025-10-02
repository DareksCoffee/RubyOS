#ifndef __UDP_H
#define __UDP_H

#include <stdint.h>
#include <stddef.h>

// UDP header structure
typedef struct udp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

// Function declarations
void udp_init();
void udp_send_packet(uint32_t dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t *payload, size_t payload_len);
void udp_receive_packet(uint8_t *packet, size_t len, uint32_t src_ip);

#endif