#ifndef NETSTACK_TCP_H
#define NETSTACK_TCP_H

#include <stdint.h>
#include <stddef.h>
#include "netbuf.h"

/* TCP Header Structure (RFC 793) */
typedef struct __attribute__((packed)) {
    uint16_t source;
    uint16_t dest;
    uint32_t seq;
    uint32_t ack_seq;
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    uint16_t res1:4,
             doff:4,
             fin:1,
             syn:1,
             rst:1,
             psh:1,
             ack:1,
             urg:1,
             ece:1,
             cwr:1;
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    uint16_t doff:4,
             res1:4,
             cwr:1,
             ece:1,
             urg:1,
             ack:1,
             psh:1,
             rst:1,
             syn:1,
             fin:1;
#else
    /* Fallback for simple little-endian (e.g., x86/ARM default) */
    uint16_t res1:4,
             doff:4,
             fin:1,
             syn:1,
             rst:1,
             psh:1,
             ack:1,
             urg:1,
             ece:1,
             cwr:1;
#endif
    uint16_t window;
    uint16_t check;
    uint16_t urg_ptr;
} tcphdr_t;

/* TCP States */
typedef enum {
    TCP_STATE_CLOSED = 0,
    TCP_STATE_LISTEN,
    TCP_STATE_SYN_SENT,
    TCP_STATE_SYN_RECEIVED,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN_WAIT_1,
    TCP_STATE_FIN_WAIT_2,
    TCP_STATE_CLOSE_WAIT,
    TCP_STATE_CLOSING,
    TCP_STATE_LAST_ACK,
    TCP_STATE_TIME_WAIT
} tcp_state_t;

/* Process incoming TCP segment */
int tcp_rx(netbuf_t *nb, uint32_t src_ip, uint32_t dst_ip);

/* Serialize and transmit a TCP segment */
int tcp_tx(int sock_id, uint32_t dst_ip, uint16_t dst_port, const uint8_t *data, uint16_t len);

#endif // NETSTACK_TCP_H
