#ifndef __ICMP_H
#define __ICMP_H

#include <stdint.h>
#include <stddef.h>

// ICMP types
#define ICMP_TYPE_ECHO_REPLY   0
#define ICMP_TYPE_ECHO_REQUEST 8

// ICMP header structure
typedef struct icmp_header {
    uint8_t type;        // ICMP type
    uint8_t code;        // ICMP code
    uint16_t checksum;   // Checksum
    uint16_t id;         // Identifier
    uint16_t seq;        // Sequence number
    // Data follows
} __attribute__((packed)) icmp_header_t;

// Function declarations
uint16_t icmp_checksum(void *data, size_t len);
void icmp_init();
void icmp_send_echo_request(uint32_t target_ip, uint16_t id, uint16_t seq, uint8_t *data, size_t data_len);
void icmp_receive_packet(uint8_t *packet, size_t len, uint32_t src_ip);

#endif