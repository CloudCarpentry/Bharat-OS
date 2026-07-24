#pragma once

#include <stdint.h>
#include "packet/packet.h"

#define UDP_HLEN 8

#pragma pack(push, 1)
typedef struct {
    uint16_t sport;
    uint16_t dport;
    uint16_t len;
    uint16_t check;
} udp_hdr_t;
#pragma pack(pop)

int udp_build_hdr(packet_t *pkt, uint16_t sport, uint16_t dport);
int udp_parse(packet_t *pkt, udp_hdr_t **hdr_out);
