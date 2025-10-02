#include "dns.h"
#include "udp.h"
#include "netdev.h"
#include "arp.h"
#include "../kernel/logger.h"
#include <string.h>

static uint16_t dns_id = 0;
static uint32_t resolved_ip = 0;
static int dns_pending = 0;

void dns_init() {
    log(LOG_LOG, "DNS initialized");
}

size_t dns_encode_name(const char *hostname, uint8_t *buffer) {
    size_t len = 0;
    const char *p = hostname;
    while (*p) {
        const char *start = p;
        while (*p && *p != '.') p++;
        uint8_t label_len = p - start;
        buffer[len++] = label_len;
        memcpy(buffer + len, start, label_len);
        len += label_len;
        if (*p == '.') p++;
    }
    buffer[len++] = 0; // Null terminator
    return len;
}

uint32_t dns_resolve(const char *hostname, uint32_t dns_server_ip) {
    // Build DNS query
    uint8_t query[512];
    size_t offset = 0;

    dns_header_t *hdr = (dns_header_t *)query;
    hdr->id = htons(dns_id++);
    hdr->flags = htons(0x0100); // Recursion desired
    hdr->qdcount = htons(1);
    hdr->ancount = 0;
    hdr->nscount = 0;
    hdr->arcount = 0;
    offset += sizeof(dns_header_t);

    // Encode name
    offset += dns_encode_name(hostname, query + offset);

    // Question
    dns_question_t *q = (dns_question_t *)(query + offset);
    q->qtype = htons(1); // A record
    q->qclass = htons(1); // IN
    offset += sizeof(dns_question_t);

    // Send via UDP
    udp_send_packet(dns_server_ip, 12345, 53, query, offset);

    // Wait for response (simple polling)
    dns_pending = 1;
    resolved_ip = 0;
    int timeout = 10000; // Arbitrary timeout
    while (dns_pending && timeout--) {
        netdev_poll();
    }

    return resolved_ip;
}

void dns_receive_packet(uint8_t *payload, size_t len, uint32_t src_ip, uint16_t src_port) {
    if (len < sizeof(dns_header_t)) return;

    dns_header_t *hdr = (dns_header_t *)payload;
    uint16_t flags = ntohs(hdr->flags);

    if ((flags & 0x8000) == 0) return; // Not a response
    if (ntohs(hdr->ancount) == 0) return; // No answers

    // Skip header
    size_t offset = sizeof(dns_header_t);

    // Skip question name
    while (offset < len && payload[offset]) {
        uint8_t label_len = payload[offset++];
        offset += label_len;
    }
    offset++; // Null
    offset += sizeof(dns_question_t); // Skip question

    // Parse answers
    for (int i = 0; i < ntohs(hdr->ancount); i++) {
        // Handle name (assume compressed 0xc00c for simplicity)
        if (payload[offset] == 0xc0) {
            offset += 2;
        } else {
            // Skip uncompressed name
            while (offset < len && payload[offset]) {
                uint8_t label_len = payload[offset++];
                offset += label_len;
            }
            offset++;
        }

        if (offset + sizeof(dns_answer_t) > len) break;

        dns_answer_t *ans = (dns_answer_t *)(payload + offset);
        offset += sizeof(dns_answer_t);

        if (ntohs(ans->type) == 1 && ntohs(ans->class) == 1) { // A record
            resolved_ip = ans->rdata;
            dns_pending = 0;
            log(LOG_LOG, "DNS resolved to %d.%d.%d.%d",
                (ntohl(resolved_ip) >> 24) & 0xFF, (ntohl(resolved_ip) >> 16) & 0xFF,
                (ntohl(resolved_ip) >> 8) & 0xFF, ntohl(resolved_ip) & 0xFF);
            return;
        }

        // Skip RDATA
        offset += ntohs(ans->rdlength);
    }
}