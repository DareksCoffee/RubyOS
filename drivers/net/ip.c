#include "ip.h"
#include "arp.h"
#include "netdev.h"
#include "icmp.h"
#include "udp.h"
#include <string.h>
#include <../kernel/logger.h>

static uint32_t local_ip = 0; // 192.168.0.1 in network order
static uint32_t default_gateway = 0; // Default gateway IP
static uint16_t ip_id = 0; // IP packet ID counter

uint16_t ip_checksum(void *data, size_t len) {
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)data;

    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }

    if (len > 0) {
        sum += *(uint8_t *)ptr;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

void ip_init() {
    local_ip = htonl(0xC0A80001); // 192.168.0.1
    default_gateway = htonl(0xC0A800FE); // 192.168.0.254
    log(LOG_LOG, "IP initialized with local IP 192.168.0.1, gateway 192.168.0.254");
}

uint32_t ip_get_local_ip() {
    return local_ip;
}

uint32_t ip_get_next_hop(uint32_t dest_ip) {
    // Simple routing: if in same subnet (192.168.0.x), send directly, else to gateway
    uint32_t subnet_mask = htonl(0xFFFFFF00); // 255.255.255.0
    if ((dest_ip & subnet_mask) == (local_ip & subnet_mask)) {
        return dest_ip;
    } else {
        return default_gateway;
    }
}

void ip_send_packet(uint32_t dest_ip, uint8_t protocol, uint8_t *payload, size_t payload_len) {
    netdev_t *dev = netdev_get_default();
    if (!dev) return;

    size_t header_len = sizeof(ip_header_t);
    size_t total_len = header_len + payload_len;
    uint8_t packet[1514]; // Max Ethernet frame

    // Build IP header
    ip_header_t *ip_hdr = (ip_header_t *)packet;
    ip_hdr->version_ihl = (4 << 4) | (header_len / 4); // Version 4, IHL 5 (20 bytes)
    ip_hdr->tos = 0;
    ip_hdr->total_len = htons(total_len);
    ip_hdr->id = htons(ip_id++);
    ip_hdr->flags_offset = 0; // No fragmentation
    ip_hdr->ttl = 64;
    ip_hdr->protocol = protocol;
    ip_hdr->checksum = 0; // Calculate after
    ip_hdr->src_ip = local_ip;
    ip_hdr->dest_ip = dest_ip;

    // Calculate checksum
    ip_hdr->checksum = ip_checksum(ip_hdr, header_len);

    // Copy payload
    memcpy(packet + header_len, payload, payload_len);

    // Determine next hop
    uint32_t next_hop = ip_get_next_hop(dest_ip);

    // Resolve MAC address
    uint8_t dest_mac[6];
    if (!arp_lookup(next_hop, dest_mac)) {
        // Send ARP request and return (packet will be sent later when ARP reply received)
        arp_send_request(ntohl(next_hop));
        log(LOG_LOG, "ARP request sent for next hop, packet queued");
        return; // For simplicity, don't queue packets yet
    }

    // Build Ethernet frame
    uint8_t frame[1514];
    memcpy(frame, dest_mac, 6); // Destination MAC
    memcpy(frame + 6, dev->mac, 6); // Source MAC
    frame[12] = (ETH_TYPE_IP >> 8) & 0xFF;
    frame[13] = ETH_TYPE_IP & 0xFF;

    // Copy IP packet
    memcpy(frame + 14, packet, total_len);

    log(LOG_LOG, "Sending IP packet to %d.%d.%d.%d, protocol %d, len %d",
        (ntohl(dest_ip) >> 24) & 0xFF, (ntohl(dest_ip) >> 16) & 0xFF,
        (ntohl(dest_ip) >> 8) & 0xFF, ntohl(dest_ip) & 0xFF,
        protocol, total_len);

    netdev_send(frame, 14 + total_len);
}

void ip_receive_packet(uint8_t *packet, size_t len) {
    if (len < sizeof(ip_header_t)) return;

    ip_header_t *ip_hdr = (ip_header_t *)packet;

    // Check if packet is for us
    if (ip_hdr->dest_ip != local_ip) {
        // Not for us, ignore for now
        return;
    }

    // Verify checksum
    uint16_t received_checksum = ip_hdr->checksum;
    ip_hdr->checksum = 0;
    uint16_t calculated_checksum = ip_checksum(ip_hdr, sizeof(ip_header_t));
    if (received_checksum != calculated_checksum) {
        log(LOG_WARNING, "IP checksum mismatch, dropping packet");
        return;
    }

    uint8_t protocol = ip_hdr->protocol;
    uint32_t src_ip = ip_hdr->src_ip;
    size_t header_len = (ip_hdr->version_ihl & 0x0F) * 4;
    uint8_t *payload = packet + header_len;
    size_t payload_len = len - header_len;

    log(LOG_LOG, "Received IP packet from %d.%d.%d.%d, protocol %d, len %d",
        (ntohl(src_ip) >> 24) & 0xFF, (ntohl(src_ip) >> 16) & 0xFF,
        (ntohl(src_ip) >> 8) & 0xFF, ntohl(src_ip) & 0xFF,
        protocol, payload_len);

    // Pass to appropriate protocol handler
    if (protocol == IP_PROTO_ICMP) {
        icmp_receive_packet(payload, payload_len, src_ip);
    } else if (protocol == IP_PROTO_UDP) {
        udp_receive_packet(payload, payload_len, src_ip);
    } else {
        log(LOG_LOG, "Received IP packet with unknown protocol %d", protocol);
    }
}

void ip_set_default_gateway(uint32_t gateway_ip) {
    default_gateway = gateway_ip;
    log(LOG_LOG, "Default gateway set to %d.%d.%d.%d",
        (ntohl(gateway_ip) >> 24) & 0xFF, (ntohl(gateway_ip) >> 16) & 0xFF,
        (ntohl(gateway_ip) >> 8) & 0xFF, ntohl(gateway_ip) & 0xFF);
}