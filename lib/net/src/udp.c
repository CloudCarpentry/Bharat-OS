#include "net/byteorder.h"
#include "net/udp.h"
#include <string.h>


int udp_build_hdr(packet_t *pkt, uint16_t sport, uint16_t dport) {
    if (!pkt) return -1;

    udp_hdr_t *udp = (udp_hdr_t *)packet_prepend(pkt, sizeof(udp_hdr_t));
    if (!udp) return -1;

    udp->sport = htons(sport);
    udp->dport = htons(dport);
    udp->len = htons(packet_length(pkt));
    udp->check = 0; /* checksum calculation omitted for simplicity */

    return 0;
}

int udp_parse(packet_t *pkt, udp_hdr_t **hdr_out) {
    if (!pkt || !hdr_out) return -1;

    if (packet_length(pkt) < sizeof(udp_hdr_t)) {
        return -1;
    }

    *hdr_out = (udp_hdr_t *)packet_data(pkt);
    packet_pull(pkt, sizeof(udp_hdr_t));

    return 0;
}
