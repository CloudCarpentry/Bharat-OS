#ifndef NETSTACK_ETHERNET_H
#define NETSTACK_ETHERNET_H

#include <stdint.h>
#include <stddef.h>
#include "netbuf.h"
#include "../../../subsys/include/bharat/network/types.h"

#define ETH_ALEN 6
#define ETH_HLEN 14

#define ETH_P_IP   0x0800
#define ETH_P_ARP  0x0806
#define ETH_P_IPV6 0x86DD

typedef struct __attribute__((packed)) {
    uint8_t h_dest[ETH_ALEN];   /* destination eth addr */
    uint8_t h_source[ETH_ALEN]; /* source ether addr    */
    uint16_t h_proto;           /* packet type ID field */
} ethhdr_t;

/* Parse incoming ethernet frame, sets pointers for upper layers, and dispatches.
   Returns 0 on success, negative error otherwise. */
int ethernet_rx(netbuf_t *nb);

/* Serialize an outgoing ethernet frame using a generic driver queue hook */
int ethernet_tx(netbuf_t *nb, const uint8_t *dest_mac, uint16_t protocol);

#endif // NETSTACK_ETHERNET_H
