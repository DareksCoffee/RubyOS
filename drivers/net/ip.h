#ifndef __IP_H
#define __IP_H

#include <stdint.h>
#include <stddef.h>

// Ethernet type for IP
#define ETH_TYPE_IP 0x0800

// IP protocol numbers
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17

// IPv4 header structure
typedef struct ip_header {
    uint8_t version_ihl;     // Version (4 bits) + IHL (4 bits)
    uint8_t tos;             // Type of Service
    uint16_t total_len;      // Total length
    uint16_t id;             // Identification
    uint16_t flags_offset;   // Flags (3 bits) + Fragment offset (13 bits)
    uint8_t ttl;             // Time to Live
    uint8_t protocol;        // Protocol
    uint16_t checksum;       // Header checksum
    uint32_t src_ip;         // Source IP address
    uint32_t dest_ip;        // Destination IP address
    // Options would go here if present
} __attribute__((packed)) ip_header_t;

// Function declarations
uint16_t ip_checksum(void *data, size_t len);
void ip_init();
uint32_t ip_get_local_ip();
void ip_send_packet(uint32_t dest_ip, uint8_t protocol, uint8_t *payload, size_t payload_len);
void ip_receive_packet(uint8_t *packet, size_t len);
void ip_set_default_gateway(uint32_t gateway_ip);

#endif