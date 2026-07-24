#include "net/netif.h"
#include "net/route.h"
#include "net/arp.h"
#include "net/ipv4.h"
#include "net/ethernet.h"
#include <stdio.h>

void netstack_tx_init(void) {
    printf("[netstack_tx] TX initialized\n");
}

// Higher-level send entry point for local services
int netstack_tx_send(ipv4_addr_t dest, packet_t *pkt, uint8_t proto) {
    ipv4_addr_t next_hop;
    netif_t *ni = route_lookup(dest, &next_hop);

    if (!ni) {
        printf("[netstack_tx] No route to host\n");
        return -1;
    }

    mac_addr_t dmac;
    if (arp_lookup(ni, next_hop, &dmac) != 0) {
        // Cache miss: enqueue packet (omitted for minimal implementation)
        // Send ARP request instead
        printf("[netstack_tx] ARP miss for next hop, sending request\n");
        arp_request(ni, next_hop);
        return -1; // Dropped for now
    }

    if (ipv4_build_hdr(pkt, ni->ip_addr, dest, proto) != 0) {
        printf("[netstack_tx] Failed to build IPv4 header\n");
        return -1;
    }

    if (eth_build_hdr(pkt, &dmac, &ni->mac, ETH_P_IP) != 0) {
        printf("[netstack_tx] Failed to build Ethernet header\n");
        return -1;
    }

    return netif_tx(ni, pkt);
}
