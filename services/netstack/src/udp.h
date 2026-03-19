#ifndef NETSTACK_UDP_H
#define NETSTACK_UDP_H

#include <stdint.h>
#include <stddef.h>
#include "netbuf.h"

typedef struct __attribute__((packed)) {
    uint16_t source;
    uint16_t dest;
    uint16_t len;
    uint16_t check;
} udphdr_t;

/* Process incoming UDP packet */
int udp_rx(netbuf_t *nb, uint32_t src_ip, uint32_t dst_ip);

/* Transmit a UDP packet from a socket */
int udp_tx(int sock_id, uint32_t dst_ip, uint16_t dst_port, const uint8_t *data, uint16_t len);

#endif // NETSTACK_UDP_H
