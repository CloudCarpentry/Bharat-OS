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

/* Core IP/ICMP checksum calculation helper */
uint16_t net_checksum(const void *data, size_t len);

/* Pseudo-header checksum calculation helper (for UDP/TCP) */
uint16_t net_pseudo_checksum(uint32_t src_ip, uint32_t dst_ip, uint8_t protocol, uint16_t udp_len);

#endif // NETSTACK_CHECKSUM_H
