#include "udp.h"
#include "ip.h"
#include "arp.h"
#include "../kernel/logger.h"
#include <string.h>

void udp_init() {
    log(LOG_LOG, "UDP initialized");
}

uint16_t udp_checksum(uint32_t src_ip, uint32_t dest_ip, uint8_t *udp_packet, size_t len) {
    // Pseudo header: src_ip (4), dest_ip (4), zero (1), protocol (1), udp_len (2)
    uint8_t pseudo[12];
    memcpy(pseudo, &src_ip, 4);
    memcpy(pseudo + 4, &dest_ip, 4);
    pseudo[8] = 0;
    pseudo[9] = IP_PROTO_UDP;
    uint16_t udp_len = len;
    memcpy(pseudo + 10, &udp_len, 2);

    // Total data: pseudo + udp_packet
    size_t total_len = 12 + len;
    uint8_t buffer[1024 + 12]; // Assume len < 1024
    memcpy(buffer, pseudo, 12);
    memcpy(buffer + 12, udp_packet, len);

    return ip_checksum(buffer, total_len);
}

void udp_send_packet(uint32_t dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t *payload, size_t payload_len) {
    size_t header_len = sizeof(udp_header_t);
    size_t total_len = header_len + payload_len;
    uint8_t packet[1024]; // Max UDP packet

    udp_header_t *hdr = (udp_header_t *)packet;
    hdr->src_port = htons(src_port);
    hdr->dest_port = htons(dest_port);
    hdr->length = htons(total_len);
    hdr->checksum = 0;

    memcpy(packet + header_len, payload, payload_len);

    // Calculate checksum
    uint32_t src_ip = ip_get_local_ip();
    hdr->checksum = udp_checksum(src_ip, dest_ip, packet, total_len);

    ip_send_packet(dest_ip, IP_PROTO_UDP, packet, total_len);
}

void udp_receive_packet(uint8_t *packet, size_t len, uint32_t src_ip) {
    if (len < sizeof(udp_header_t)) return;

    udp_header_t *hdr = (udp_header_t *)packet;
    uint16_t dest_port = ntohs(hdr->dest_port);
    uint16_t src_port = ntohs(hdr->src_port);
    uint16_t udp_len = ntohs(hdr->length);

    if (udp_len > len) return;

    uint8_t *payload = packet + sizeof(udp_header_t);
    size_t payload_len = udp_len - sizeof(udp_header_t);

    // Verify checksum
    uint16_t received_checksum = hdr->checksum;
    if (received_checksum != 0) {
        uint32_t dest_ip = ip_get_local_ip();
        uint16_t calculated = udp_checksum(src_ip, dest_ip, packet, udp_len);
        if (calculated != 0) {
            log(LOG_WARNING, "UDP checksum mismatch, dropping packet");
            return;
        }
    }

    log(LOG_LOG, "Received UDP packet from %d.%d.%d.%d:%d to port %d, len %d",
        (ntohl(src_ip) >> 24) & 0xFF, (ntohl(src_ip) >> 16) & 0xFF,
        (ntohl(src_ip) >> 8) & 0xFF, ntohl(src_ip) & 0xFF,
        src_port, dest_port, payload_len);

    // Dispatch to handlers (for now, only DNS on port 53)
    if (dest_port == 53) {
        // Call DNS receive
        extern void dns_receive_packet(uint8_t *payload, size_t len, uint32_t src_ip, uint16_t src_port);
        dns_receive_packet(payload, payload_len, src_ip, src_port);
    } else {
        log(LOG_LOG, "UDP packet to unknown port %d", dest_port);
    }
}