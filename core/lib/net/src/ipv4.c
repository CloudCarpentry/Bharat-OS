#include "net/byteorder.h"
#include "net/ipv4.h"
#include "net/checksum.h"
#include <string.h>

int ipv4_build_hdr(packet_t *pkt, ipv4_addr_t src, ipv4_addr_t dst, uint8_t proto) {
    if (!pkt) return -1;

    ipv4_hdr_t *ip = (ipv4_hdr_t *)packet_prepend(pkt, sizeof(ipv4_hdr_t));
    if (!ip) return -1;

    ip->version = IP_VERSION_4;
    ip->ihl = sizeof(ipv4_hdr_t) / 4;
    ip->tos = 0;
    ip->tot_len = htons(packet_length(pkt));
    ip->id = htons(0); /* Unfragmented */
    ip->frag_off = htons(0x4000); /* DF bit */
    ip->ttl = 64;
    ip->protocol = proto;
    ip->saddr = src.addr;
    ip->daddr = dst.addr;
    ip->check = 0;
    ip->check = net_checksum(ip, sizeof(ipv4_hdr_t));

    return 0;
}

int ipv4_parse(packet_t *pkt, ipv4_hdr_t **hdr_out) {
    if (!pkt || !hdr_out) return -1;

    if (packet_length(pkt) < sizeof(ipv4_hdr_t)) {
        return -1;
    }

    ipv4_hdr_t *ip = (ipv4_hdr_t *)packet_data(pkt);
    if (ip->version != IP_VERSION_4) {
        return -1;
    }

    uint16_t hlen = ip->ihl * 4;
    if (hlen < sizeof(ipv4_hdr_t) || hlen > packet_length(pkt)) {
        return -1;
    }

    *hdr_out = ip;
    packet_pull(pkt, hlen);

    return 0;
}
