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

/* Core internet checksum algorithm (RFC 1071)
 * Optimized to use a wide-word 64-bit accumulator on 64-bit chunks.
 */
uint16_t net_checksum(const void *data, size_t len) {
    uint64_t sum = 0;
    const uint8_t *ptr = (const uint8_t *)data;

    // Process 64-bit chunks. A 64-bit accumulator on 64-bit chunk additions
    // can overflow if not folded. However, the maximum number of 64-bit words
    // in a 64KB packet is 8192. So 8192 additions of 0xFFFFFFFFFFFFFFFF
    // will produce a maximum value of 8192 * (2^64 - 1), which fits in 77 bits.
    // Since we only have a 64-bit accumulator, we MUST fold intermediate carries
    // or risk overflow if we just use naive addition without a wider type (e.g. __int128).
    // An alternative is to add 64-bit words and immediately add the carry back,
    // or use a safe inline assembly block for `adcq`. To remain fully portable C,
    // we manually fold carries continuously.

    while (len >= 8) {
        uint64_t word = (uint64_t)ptr[0] | ((uint64_t)ptr[1] << 8) |
                        ((uint64_t)ptr[2] << 16) | ((uint64_t)ptr[3] << 24) |
                        ((uint64_t)ptr[4] << 32) | ((uint64_t)ptr[5] << 40) |
                        ((uint64_t)ptr[6] << 48) | ((uint64_t)ptr[7] << 56);

        sum += word;
        // Check for overflow (carry)
        if (sum < word) {
            sum++;
        }

        ptr += 8;
        len -= 8;
    }

    // Process remaining 32-bit chunk
    if (len >= 4) {
        uint32_t word = (uint32_t)ptr[0] | ((uint32_t)ptr[1] << 8) |
                        ((uint32_t)ptr[2] << 16) | ((uint32_t)ptr[3] << 24);
        sum += word;
        if (sum < word) {
            sum++;
        }
        ptr += 4;
        len -= 4;
    }

    // Process remaining 16-bit chunk
    if (len >= 2) {
        uint16_t word = (uint16_t)ptr[0] | ((uint16_t)ptr[1] << 8);
        sum += word;
        if (sum < word) {
            sum++;
        }
        ptr += 2;
        len -= 2;
    }

    // Process remaining byte
    if (len == 1) {
        sum += ptr[0];
        if (sum < ptr[0]) {
            sum++;
        }
    }

    // Fold 64-bit sum into 32-bit
    while (sum >> 32) {
        sum = (sum & 0xFFFFFFFF) + (sum >> 32);
    }

    // Fold 32-bit sum into 16-bit
    uint32_t sum32 = (uint32_t)sum;
    while (sum32 >> 16) {
        sum32 = (sum32 & 0xFFFF) + (sum32 >> 16);
    }

    return (uint16_t)~sum32;
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
