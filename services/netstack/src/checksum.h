#ifndef NETSTACK_CHECKSUM_H
#define NETSTACK_CHECKSUM_H

#include <stdint.h>
#include <stddef.h>

/* Basic byte-order helpers for Phase 2.
   Assuming little-endian host (x86_64/arm64/riscv64) for simplicity in this baseline. */
uint16_t bnet_htons(uint16_t v);
uint16_t bnet_ntohs(uint16_t v);
uint32_t bnet_htonl(uint32_t v);
uint32_t bnet_ntohl(uint32_t v);

/* Core IP/ICMP checksum calculation helpers */
uint32_t net_csum_partial(const void *buf, size_t len, uint32_t sum);
uint16_t net_csum_finalize(uint32_t sum);

uint32_t net_csum_partialv(const void *buf1, size_t len1,
                           const void *buf2, size_t len2,
                           uint32_t sum);

/* IPv4 pseudo header accumulation only; does not finalize. */
uint32_t net_csum_ipv4_pseudo_partial(uint32_t src_ip_be,
                                      uint32_t dst_ip_be,
                                      uint8_t protocol,
                                      uint16_t l4_len_be,
                                      uint32_t sum);

/* Convenience wrappers for complete L4 checksums. */
uint16_t net_csum_udp_ipv4(const void *udp_hdr_and_payload,
                           size_t udp_len,
                           uint32_t src_ip_be,
                           uint32_t dst_ip_be);

uint16_t net_csum_tcp_ipv4(const void *tcp_hdr_and_payload,
                           size_t tcp_len,
                           uint32_t src_ip_be,
                           uint32_t dst_ip_be);

#endif // NETSTACK_CHECKSUM_H
