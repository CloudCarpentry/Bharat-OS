#include "net/byteorder.h"
#include "net/ethernet.h"
#include <string.h>


int eth_build_hdr(packet_t *pkt, const mac_addr_t *dst, const mac_addr_t *src, uint16_t proto) {
    if (!pkt || !dst || !src) return -1;

    /* Ensure room for ethernet header */
    eth_hdr_t *eth = (eth_hdr_t *)packet_prepend(pkt, sizeof(eth_hdr_t));
    if (!eth) return -1; /* Packet buffer too small */

    memcpy(eth->h_dest, dst->addr, ETH_ALEN);
    memcpy(eth->h_source, src->addr, ETH_ALEN);
    eth->h_proto = htons(proto);

    return 0;
}

int eth_parse(packet_t *pkt, eth_hdr_t **hdr_out) {
    if (!pkt || !hdr_out) return -1;

    if (packet_length(pkt) < sizeof(eth_hdr_t)) {
        return -1; /* Truncated frame */
    }

    *hdr_out = (eth_hdr_t *)packet_data(pkt);

    /* Advance payload pointer past ethernet header */
    packet_pull(pkt, sizeof(eth_hdr_t));

    return 0;
}
