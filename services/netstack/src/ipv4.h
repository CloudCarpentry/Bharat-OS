#ifndef NETSTACK_IPV4_H
#define NETSTACK_IPV4_H

#include <stdint.h>
#include <stddef.h>
#include "netbuf.h"

#define IPPROTO_ICMP 1
#define IPPROTO_TCP  6
#define IPPROTO_UDP  17

typedef struct __attribute__((packed)) {
    uint8_t  ihl:4,
             version:4;
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

/* Configuration / query API */
int ipv4_set_local_ip(uint32_t ip);
uint32_t ipv4_get_local_ip(void);
uint32_t ipv4_get_loopback_ip(void);
uint32_t ipv4_select_source_ip(uint32_t dst_ip);
int ipv4_is_local_address(uint32_t ip);

#endif // NETSTACK_IPV4_H
