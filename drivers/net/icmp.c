#include "icmp.h"
#include "ip.h"
#include "arp.h"
#include <string.h>
#include <../kernel/logger.h>

uint16_t icmp_checksum(void *data, size_t len) {
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

void icmp_init() {
    log(LOG_LOG, "ICMP initialized");
}

void icmp_send_echo_request(uint32_t target_ip, uint16_t id, uint16_t seq, uint8_t *data, size_t data_len) {
    size_t icmp_len = sizeof(icmp_header_t) + data_len;
    uint8_t icmp_packet[icmp_len];

    icmp_header_t *icmp_hdr = (icmp_header_t *)icmp_packet;
    icmp_hdr->type = ICMP_TYPE_ECHO_REQUEST;
    icmp_hdr->code = 0;
    icmp_hdr->checksum = 0;
    icmp_hdr->id = id;
    icmp_hdr->seq = seq;

    // Copy data
    memcpy(icmp_packet + sizeof(icmp_header_t), data, data_len);

    // Calculate checksum
    icmp_hdr->checksum = icmp_checksum(icmp_packet, icmp_len);

    log(LOG_LOG, "Sending ICMP echo request to %d.%d.%d.%d, id=%d, seq=%d",
        (ntohl(target_ip) >> 24) & 0xFF, (ntohl(target_ip) >> 16) & 0xFF,
        (ntohl(target_ip) >> 8) & 0xFF, ntohl(target_ip) & 0xFF,
        id, seq);

    ip_send_packet(target_ip, IP_PROTO_ICMP, icmp_packet, icmp_len);
}

void icmp_receive_packet(uint8_t *packet, size_t len, uint32_t src_ip) {
    if (len < sizeof(icmp_header_t)) return;

    icmp_header_t *icmp_hdr = (icmp_header_t *)packet;

    // Verify checksum
    uint16_t received_checksum = icmp_hdr->checksum;
    icmp_hdr->checksum = 0;
    uint16_t calculated_checksum = icmp_checksum(packet, len);
    if (received_checksum != calculated_checksum) {
        log(LOG_WARNING, "ICMP checksum mismatch, dropping packet");
        return;
    }

    if (icmp_hdr->type == ICMP_TYPE_ECHO_REQUEST) {
        // Send echo reply
        icmp_hdr->type = ICMP_TYPE_ECHO_REPLY;
        icmp_hdr->checksum = 0;
        icmp_hdr->checksum = icmp_checksum(packet, len);

        log(LOG_LOG, "Sending ICMP echo reply to %d.%d.%d.%d, id=%d, seq=%d",
            (ntohl(src_ip) >> 24) & 0xFF, (ntohl(src_ip) >> 16) & 0xFF,
            (ntohl(src_ip) >> 8) & 0xFF, ntohl(src_ip) & 0xFF,
            icmp_hdr->id, icmp_hdr->seq);

        ip_send_packet(src_ip, IP_PROTO_ICMP, packet, len);
    } else if (icmp_hdr->type == ICMP_TYPE_ECHO_REPLY) {
        log(LOG_LOG, "Received ICMP echo reply from %d.%d.%d.%d, id=%d, seq=%d",
            (ntohl(src_ip) >> 24) & 0xFF, (ntohl(src_ip) >> 16) & 0xFF,
            (ntohl(src_ip) >> 8) & 0xFF, ntohl(src_ip) & 0xFF,
            icmp_hdr->id, icmp_hdr->seq);
    } else {
        log(LOG_LOG, "Received ICMP packet type %d from %d.%d.%d.%d",
            icmp_hdr->type,
            (ntohl(src_ip) >> 24) & 0xFF, (ntohl(src_ip) >> 16) & 0xFF,
            (ntohl(src_ip) >> 8) & 0xFF, ntohl(src_ip) & 0xFF);
    }
}