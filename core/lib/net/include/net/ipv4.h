#pragma once

#include <stdint.h>
#include "packet/packet.h"
#include "net/netif.h"

#define IP_VERSION_4 4
#define IP_HLEN_MIN 20
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

#pragma pack(push, 1)
typedef struct {
    uint8_t  ihl:4, version:4;
    uint8_t  tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
} ipv4_hdr_t;
#pragma pack(pop)

int ipv4_build_hdr(packet_t *pkt, ipv4_addr_t src, ipv4_addr_t dst, uint8_t proto);
int ipv4_parse(packet_t *pkt, ipv4_hdr_t **hdr_out);
