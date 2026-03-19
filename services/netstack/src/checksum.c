#include "checksum.h"

// Basic byte-order helpers
// Since we're targeting x86_64, arm64, riscv64, all are little-endian.
// We must byte-swap for network order (big-endian).

uint16_t bnet_htons(uint16_t v) {
    return (v >> 8) | (v << 8);
}

uint16_t bnet_ntohs(uint16_t v) {
    return bnet_htons(v);
}

uint32_t bnet_htonl(uint32_t v) {
    return ((v >> 24) & 0xff) |
           ((v << 8) & 0xff0000) |
           ((v >> 8) & 0xff00) |
           ((v << 24) & 0xff000000);
}

uint32_t bnet_ntohl(uint32_t v) {
    return bnet_htonl(v);
}

/* Core internet checksum algorithm (RFC 1071) */
uint16_t net_checksum(const void *data, size_t len) {
    const uint16_t *buf = (const uint16_t *)data;
    uint32_t sum = 0;

    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    if (len == 1) {
        uint16_t last_byte = 0;
        *(uint8_t *)&last_byte = *(const uint8_t *)buf;
        sum += last_byte;
    }

    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return ~sum;
}

/* Pseudo-header sum for UDP checksums */
uint16_t net_pseudo_checksum(uint32_t src_ip, uint32_t dst_ip, uint8_t protocol, uint16_t udp_len) {
    uint32_t sum = 0;

    sum += (src_ip >> 16) & 0xffff;
    sum += src_ip & 0xffff;
    sum += (dst_ip >> 16) & 0xffff;
    sum += dst_ip & 0xffff;
    sum += bnet_htons((uint16_t)protocol);
    sum += bnet_htons(udp_len);

    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return sum; // Do not invert yet, to allow chained sum with UDP payload
}
