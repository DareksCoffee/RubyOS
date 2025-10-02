#ifndef __DNS_H
#define __DNS_H

#include <stdint.h>
#include <stddef.h>

// DNS header structure
typedef struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed)) dns_header_t;

// DNS question structure (after name)
typedef struct dns_question {
    uint16_t qtype;
    uint16_t qclass;
} __attribute__((packed)) dns_question_t;

// DNS answer structure (after name)
typedef struct dns_answer {
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rdlength;
    uint32_t rdata; // For A record, IPv4 address
} __attribute__((packed)) dns_answer_t;

// Function declarations
void dns_init();
uint32_t dns_resolve(const char *hostname, uint32_t dns_server_ip);
void dns_receive_packet(uint8_t *payload, size_t len, uint32_t src_ip, uint16_t src_port);

#endif