#pragma once

#include <stdint.h>
#include "packet/packet.h"
#include "net/netif.h"

#define ETH_ALEN 6
#define ETH_HLEN 14
#define ETH_P_IP 0x0800
#define ETH_P_ARP 0x0806
#define ETH_P_IPV6 0x86DD

#pragma pack(push, 1)
typedef struct {
    uint8_t h_dest[ETH_ALEN]; /* destination eth addr */
    uint8_t h_source[ETH_ALEN]; /* source ether addr */
    uint16_t h_proto; /* packet type ID field */
} eth_hdr_t;
#pragma pack(pop)

/* Prepare an ethernet header in the given packet buffer */
int eth_build_hdr(packet_t *pkt, const mac_addr_t *dst, const mac_addr_t *src, uint16_t proto);

/* Parse an incoming ethernet frame, advancing packet pointers past header */
int eth_parse(packet_t *pkt, eth_hdr_t **hdr_out);
