#ifndef NETSTACK_ICMP_H
#define NETSTACK_ICMP_H

#include <stdint.h>
#include <stddef.h>
#include "netbuf.h"

#define ICMP_ECHO_REPLY   0
#define ICMP_UNREACH      3
#define ICMP_ECHO         8

#define ICMP_UNREACH_PROTOCOL 2
#define ICMP_UNREACH_PORT     3

typedef struct __attribute__((packed)) {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    union {
        struct {
            uint16_t id;
            uint16_t sequence;
        } echo;
        uint32_t gateway;
        struct {
            uint16_t __unused;
            uint16_t mtu;
        } frag;
    } un;
} icmphdr_t;

/* Process incoming ICMP packet */
int icmp_rx(netbuf_t *nb, uint32_t src_ip, uint32_t dst_ip);

/* Send an ICMP Unreachable message (e.g., for unsupported protocol or port) */
int icmp_send_unreachable(netbuf_t *nb, uint32_t src_ip, uint32_t dst_ip, uint8_t code);

/* Send an ICMP Echo Reply */
int icmp_send_echo_reply(netbuf_t *nb, uint32_t src_ip, uint32_t dst_ip, uint16_t id, uint16_t seq);

#endif // NETSTACK_ICMP_H
