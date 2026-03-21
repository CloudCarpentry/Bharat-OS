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
uint32_t net_csum_partial(const void *buf, size_t len, uint32_t sum_in) {
    uint64_t sum = sum_in;
    const uint8_t *ptr = (const uint8_t *)buf;

    while (len >= 8) {
        uint64_t word = (uint64_t)ptr[0] | ((uint64_t)ptr[1] << 8) |
                        ((uint64_t)ptr[2] << 16) | ((uint64_t)ptr[3] << 24) |
                        ((uint64_t)ptr[4] << 32) | ((uint64_t)ptr[5] << 40) |
                        ((uint64_t)ptr[6] << 48) | ((uint64_t)ptr[7] << 56);

        sum += word;
        if (sum < word) {
            sum++;
        }
        ptr += 8;
        len -= 8;
    }

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

    if (len >= 2) {
        uint16_t word = (uint16_t)ptr[0] | ((uint16_t)ptr[1] << 8);
        sum += word;
        if (sum < word) {
            sum++;
        }
        ptr += 2;
        len -= 2;
    }

    if (len == 1) {
        // Correct handling of an odd byte
        // In network byte order (which we accumulate in little-endian locally),
        // the last odd byte is in the high byte of the next 16-bit word if we
        // think about Big Endian memory layout, but here we treat it as the
        // low byte locally. Wait, RFC 1071 states if there's an odd byte, it is
        // logically padded with a zero byte at the end.
        // So in our little-endian addition logic where ptr[0] is the low byte:
        // the padded word would be ptr[0] | (0 << 8). So simply adding ptr[0] is correct!
        sum += ptr[0];
        if (sum < ptr[0]) {
            sum++;
        }
    }

    while (sum >> 32) {
        sum = (sum & 0xFFFFFFFF) + (sum >> 32);
    }

    return (uint32_t)sum;
}

uint16_t net_csum_finalize(uint32_t sum) {
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (uint16_t)~sum;
}

uint32_t net_csum_partialv(const void *buf1, size_t len1,
                           const void *buf2, size_t len2,
                           uint32_t sum) {
    uint32_t s = net_csum_partial(buf1, len1, sum);
    // If len1 is odd, the last byte of buf1 was treated as the low byte of a 16-bit word.
    // The first byte of buf2 must then be treated as the high byte of that same 16-bit word.
    // To fix this without complex state, we can just swap the bytes of the sum from buf2
    // before adding it to s, if len1 is odd.
    uint32_t s2 = net_csum_partial(buf2, len2, 0);
    if (len1 & 1) {
        // Since net_csum_partial already folded it down to at most 0xFFFFFFFF,
        // we swap the 16-bit word bytes.
        // E.g., if s2 = 0x0000AABB, we want 0x0000BBAA.
        // First fold it to 16-bits to avoid losing upper bits on swap
        while (s2 >> 16) {
            s2 = (s2 & 0xFFFF) + (s2 >> 16);
        }
        s2 = ((s2 & 0xFF00) >> 8) | ((s2 & 0x00FF) << 8);
    }

    // We must return a 32-bit partial sum. Folding to 16 bits here would break
    // chained accumulations if we want to retain 32-bit carry capacity.
    uint64_t sum64 = (uint64_t)s + s2;
    while (sum64 >> 32) {
        sum64 = (sum64 & 0xFFFFFFFF) + (sum64 >> 32);
    }
    return (uint32_t)sum64;
}

uint32_t net_csum_ipv4_pseudo_partial(uint32_t src_ip_be,
                                      uint32_t dst_ip_be,
                                      uint8_t protocol,
                                      uint16_t l4_len_be,
                                      uint32_t sum) {
    sum += (src_ip_be >> 16) & 0xffff;
    sum += src_ip_be & 0xffff;
    sum += (dst_ip_be >> 16) & 0xffff;
    sum += dst_ip_be & 0xffff;
    sum += bnet_htons((uint16_t)protocol);
    sum += l4_len_be; // already big-endian (network order), but we add as-is like the rest

    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return sum;
}

uint16_t net_csum_udp_ipv4(const void *udp_hdr_and_payload,
                           size_t udp_len,
                           uint32_t src_ip_be,
                           uint32_t dst_ip_be) {
    uint32_t sum = net_csum_ipv4_pseudo_partial(src_ip_be, dst_ip_be, 17 /* IPPROTO_UDP */, bnet_htons((uint16_t)udp_len), 0);
    sum = net_csum_partial(udp_hdr_and_payload, udp_len, sum);
    uint16_t res = net_csum_finalize(sum);
    return res == 0 ? 0xFFFF : res; // UDP special case
}

uint16_t net_csum_tcp_ipv4(const void *tcp_hdr_and_payload,
                           size_t tcp_len,
                           uint32_t src_ip_be,
                           uint32_t dst_ip_be) {
    uint32_t sum = net_csum_ipv4_pseudo_partial(src_ip_be, dst_ip_be, 6 /* IPPROTO_TCP */, bnet_htons((uint16_t)tcp_len), 0);
    sum = net_csum_partial(tcp_hdr_and_payload, tcp_len, sum);
    return net_csum_finalize(sum);
}
