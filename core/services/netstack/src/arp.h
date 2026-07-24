#ifndef NETSTACK_ARP_H
#define NETSTACK_ARP_H

#include <stdint.h>
#include <stddef.h>
#include "netbuf.h"

#define ARP_HRD_ETHER 1
#define ARP_PRO_IP    0x0800
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY   2

typedef struct __attribute__((packed)) {
    uint16_t ar_hrd;      /* format of hardware address   */
    uint16_t ar_pro;      /* format of protocol address   */
    uint8_t  ar_hln;      /* length of hardware address   */
    uint8_t  ar_pln;      /* length of protocol address   */
    uint16_t ar_op;       /* ARP opcode (command)         */
} arphdr_t;

typedef struct __attribute__((packed)) {
    uint8_t  ar_sha[6];   /* sender hardware address      */
    uint32_t ar_sip;      /* sender IP address            */
    uint8_t  ar_tha[6];   /* target hardware address      */
    uint32_t ar_tip;      /* target IP address            */
} arphdr_payload_t;

/* Process incoming ARP packet (from Ethernet layer) */
int arp_rx(netbuf_t *nb);

/* Resolve an IP address to a MAC address, sending an ARP request if necessary.
   For Phase 2, this is a simplified blocking or direct table lookup.
   Returns 0 on success (MAC placed in out_mac), negative on failure. */
int arp_resolve(uint32_t target_ip, uint8_t *out_mac);

/* Send an ARP reply */
int arp_send_reply(uint32_t target_ip, const uint8_t *target_mac);

/* Send an ARP request */
int arp_send_request(uint32_t target_ip);

#endif // NETSTACK_ARP_H
