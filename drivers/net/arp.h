#ifndef __ARP_H
#define __ARP_H

#include <stdint.h>
#include <stddef.h>

// Byte order conversion functions
static inline uint16_t htons(uint16_t x) {
    return (x >> 8) | (x << 8);
}

static inline uint16_t ntohs(uint16_t x) {
    return htons(x);
}

static inline uint32_t htonl(uint32_t x) {
    return ((x >> 24) & 0xFF) | ((x >> 8) & 0xFF00) | ((x << 8) & 0xFF0000) | ((x << 24) & 0xFF000000);
}

static inline uint32_t ntohl(uint32_t x) {
    return htonl(x);
}

#define ETH_TYPE_ARP 0x0806

#define ARP_REQUEST 0x0001
#define ARP_REPLY   0x0002

#define ARP_HTYPE_ETHERNET 0x0001

#define ARP_PTYPE_IPV4 0x0800

#define ARP_HLEN 6

#define ARP_PLEN 4

// ARP packet structure
typedef struct arp_packet {
    uint16_t htype;      // Hardware type
    uint16_t ptype;      // Protocol type
    uint8_t hlen;        // Hardware address length
    uint8_t plen;        // Protocol address length
    uint16_t opcode;     // Operation code
    uint8_t sender_mac[6]; // Sender hardware address
    uint32_t sender_ip;  // Sender protocol address
    uint8_t target_mac[6]; // Target hardware address
    uint32_t target_ip;  // Target protocol address
} __attribute__((packed)) arp_packet_t;

#define ARP_CACHE_SIZE 16

typedef struct arp_cache_entry {
    uint32_t ip;
    uint8_t mac[6];
    int valid; // 1 if entry is valid
} arp_cache_entry_t;


void arp_init();
void arp_send_request(uint32_t target_ip);
void arp_handle_reply(arp_packet_t *arp);
int arp_lookup(uint32_t ip, uint8_t *mac);
void arp_add_entry(uint32_t ip, uint8_t *mac);

#endif