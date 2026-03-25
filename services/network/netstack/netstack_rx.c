#include "net/byteorder.h"
#include "net/netif.h"
#include "net/ethernet.h"
#include "net/arp.h"
#include "net/ipv4.h"
#include "net/udp.h"
#include <stdio.h>

void netstack_rx_init(void) {
    printf("[netstack_rx] RX initialized\n");
}

void netstack_rx_handle_udp(netif_t *ni, packet_t *pkt, ipv4_hdr_t *ip) {
    (void)ni;
    (void)ip;
    udp_hdr_t *udp;
    if (udp_parse(pkt, &udp) == 0) {
        printf("[netstack_rx] UDP packet received: src port %u, dst port %u, len %u\n",
               ntohs(udp->sport), ntohs(udp->dport), ntohs(udp->len));
    } else {
        printf("[netstack_rx] Failed to parse UDP header\n");
    }
}

void netstack_rx_handle_ipv4(netif_t *ni, packet_t *pkt) {
    ipv4_hdr_t *ip;
    if (ipv4_parse(pkt, &ip) == 0) {
        printf("[netstack_rx] IPv4 packet received: proto %u, src %08x, dst %08x\n",
               ip->protocol, ip->saddr, ip->daddr);

        if (ip->protocol == IP_PROTO_UDP) {
            netstack_rx_handle_udp(ni, pkt, ip);
        } else {
            printf("[netstack_rx] Unhandled IPv4 protocol %u\n", ip->protocol);
        }
    } else {
        printf("[netstack_rx] Failed to parse IPv4 header\n");
    }
}

void netstack_rx_packet(netif_t *ni, packet_t *pkt) {
    eth_hdr_t *eth;
    if (eth_parse(pkt, &eth) == 0) {
        uint16_t proto = ntohs(eth->h_proto);
        if (proto == ETH_P_ARP) {
            printf("[netstack_rx] Handling ARP packet\n");
            arp_input(ni, pkt);
        } else if (proto == ETH_P_IP) {
            printf("[netstack_rx] Handling IPv4 packet\n");
            netstack_rx_handle_ipv4(ni, pkt);
        } else {
            printf("[netstack_rx] Unhandled Ethernet protocol 0x%04x\n", proto);
        }
    } else {
        printf("[netstack_rx] Failed to parse Ethernet header\n");
    }
    packet_free(pkt);
}
