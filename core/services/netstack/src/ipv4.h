#ifndef NETSTACK_IPV4_H
#define NETSTACK_IPV4_H

#include <stdint.h>
#include <stddef.h>
#include "netbuf.h"

#define IPPROTO_ICMP 1
#define IPPROTO_TCP  6
#define IPPROTO_UDP  17

/*
 * IPV4_ADDR creates a uint32_t IP address in host byte order.
 * Since the stack consistently expects host byte order internally
 * (to easily do operations like subnet masking), we construct it based
 * on the CPU's endianness.
 */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define IPV4_ADDR(a, b, c, d) \
    (((uint32_t)(d) << 24) | ((uint32_t)(c) << 16) | ((uint32_t)(b) << 8) | ((uint32_t)(a)))
#else
#define IPV4_ADDR(a, b, c, d) \
    (((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(c) << 8) | ((uint32_t)(d)))
#endif

typedef struct __attribute__((packed)) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint8_t  ihl:4,
             version:4;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint8_t  version:4,
             ihl:4;
#else
#error "Byte order not supported"
#endif
    uint8_t  tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
} iphdr_t;

/* Process incoming IPv4 packet */
int ipv4_rx(netbuf_t *nb);

/* Serialize and transmit an IPv4 packet */
int ipv4_tx(netbuf_t *nb, uint32_t dst_ip, uint8_t protocol);

/* Centralized IP routing and configuration
 * Note: IP addresses are expected in host byte order.
 * Use the IPV4_ADDR macro to construct them safely.
 */
uint32_t ipv4_get_source_ip(uint32_t dst_ip);

/* Configuration / query API */
int ipv4_set_local_ip(uint32_t ip);
uint32_t ipv4_get_local_ip(void);
uint32_t ipv4_get_loopback_ip(void);
int ipv4_is_local_address(uint32_t ip);

#endif // NETSTACK_IPV4_H
