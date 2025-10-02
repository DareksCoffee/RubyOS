#include "arp.h"
#include "netdev.h"
#include <string.h>
#include <../kernel/logger.h>

static arp_cache_entry_t arp_cache[ARP_CACHE_SIZE];
static uint32_t local_ip = 0; // To be set, e.g., 192.168.0.1 in network order

void arp_init() {
    memset(arp_cache, 0, sizeof(arp_cache));
    // Set local IP, for example
    local_ip = htonl(0xC0A80001);
    log(LOG_LOG, "ARP initialized with local IP 192.168.0.1");
}

void arp_send_request(uint32_t target_ip) {
    netdev_t *dev = netdev_get_default();
    if (!dev) return;

    uint8_t frame[42]; // Ethernet header (14) + ARP (28)

    // Ethernet header
    memset(frame, 0xFF, 6); // Broadcast destination MAC
    memcpy(frame + 6, dev->mac, 6); // Source MAC
    frame[12] = (ETH_TYPE_ARP >> 8) & 0xFF;
    frame[13] = ETH_TYPE_ARP & 0xFF;

    // ARP packet
    arp_packet_t *arp = (arp_packet_t *)(frame + 14);
    arp->htype = htons(ARP_HTYPE_ETHERNET);
    arp->ptype = htons(ARP_PTYPE_IPV4);
    arp->hlen = ARP_HLEN;
    arp->plen = ARP_PLEN;
    arp->opcode = htons(ARP_REQUEST);
    memcpy(arp->sender_mac, dev->mac, 6);
    arp->sender_ip = local_ip;
    memset(arp->target_mac, 0, 6);
    arp->target_ip = htonl(target_ip);

    log(LOG_LOG, "Sending ARP request for IP %d.%d.%d.%d",
        (target_ip >> 24) & 0xFF, (target_ip >> 16) & 0xFF,
        (target_ip >> 8) & 0xFF, target_ip & 0xFF);

    netdev_send(frame, sizeof(frame));
}

void arp_handle_reply(arp_packet_t *arp) {
    if (arp->target_ip == local_ip) {
        uint32_t ip = ntohl(arp->sender_ip);
        log(LOG_LOG, "Received ARP reply from IP %d.%d.%d.%d with MAC %02x:%02x:%02x:%02x:%02x:%02x",
            (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF,
            arp->sender_mac[0], arp->sender_mac[1], arp->sender_mac[2],
            arp->sender_mac[3], arp->sender_mac[4], arp->sender_mac[5]);
        arp_add_entry(ip, arp->sender_mac);
    }
}

int arp_lookup(uint32_t ip, uint8_t *mac) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            memcpy(mac, arp_cache[i].mac, 6);
            return 1;
        }
    }
    return 0;
}

void arp_add_entry(uint32_t ip, uint8_t *mac) {
    // Check if entry exists
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            memcpy(arp_cache[i].mac, mac, 6);
            return;
        }
    }
    // Find free slot
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!arp_cache[i].valid) {
            arp_cache[i].ip = ip;
            memcpy(arp_cache[i].mac, mac, 6);
            arp_cache[i].valid = 1;
            return;
        }
    }
    // Overwrite first entry if full
    arp_cache[0].ip = ip;
    memcpy(arp_cache[0].mac, mac, 6);
    arp_cache[0].valid = 1;
}