#include "net/byteorder.h"
#include "net/arp.h"
#include <string.h>


#define ARP_CACHE_SIZE 16

typedef struct {
    ipv4_addr_t ip;
    mac_addr_t mac;
    int state; /* 0=empty, 1=pending, 2=resolved */
    uint32_t timestamp;
} arp_entry_t;

static arp_entry_t arp_cache[ARP_CACHE_SIZE];

static void arp_update_cache(ipv4_addr_t ip, const mac_addr_t *mac) {
    if (!mac) return;
    for (int i=0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].state == 2 && arp_cache[i].ip.addr == ip.addr) {
            memcpy(&arp_cache[i].mac, mac, sizeof(mac_addr_t));
            return;
        }
    }
    for (int i=0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].state == 0) {
            arp_cache[i].ip = ip;
            memcpy(&arp_cache[i].mac, mac, sizeof(mac_addr_t));
            arp_cache[i].state = 2;
            return;
        }
    }
    /* LRU eviction omitted for simplicity */
}

int arp_lookup(netif_t *ni, ipv4_addr_t ip, mac_addr_t *mac_out) {
    (void)ni;
    for (int i=0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].state == 2 && arp_cache[i].ip.addr == ip.addr) {
            if (mac_out) memcpy(mac_out, &arp_cache[i].mac, sizeof(mac_addr_t));
            return 0;
        }
    }
    return -1; /* Cache miss */
}

int arp_request(netif_t *ni, ipv4_addr_t target_ip) {
    if (!ni || !ni->is_up) return -1;
    packet_t *pkt = packet_alloc(sizeof(arp_hdr_t) + sizeof(eth_hdr_t));
    if (!pkt) return -1;

    packet_reserve(pkt, sizeof(eth_hdr_t));
    arp_hdr_t *arp = (arp_hdr_t *)packet_append(pkt, sizeof(arp_hdr_t));

    arp->ar_hrd = htons(ARPHRD_ETHER);
    arp->ar_pro = htons(ETH_P_IP);
    arp->ar_hln = ETH_ALEN;
    arp->ar_pln = 4;
    arp->ar_op  = htons(ARPOP_REQUEST);

    memcpy(arp->ar_sha, ni->mac.addr, ETH_ALEN);
    memcpy(arp->ar_spa, &ni->ip_addr.addr, 4);
    memset(arp->ar_tha, 0, ETH_ALEN);
    memcpy(arp->ar_tpa, &target_ip.addr, 4);

    mac_addr_t bcast = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
    eth_build_hdr(pkt, &bcast, &ni->mac, ETH_P_ARP);

    int ret = netif_tx(ni, pkt);
    if (ret != 0) packet_free(pkt);
    return ret;
}

int arp_input(netif_t *ni, packet_t *pkt) {
    if (packet_length(pkt) < sizeof(arp_hdr_t)) {
        return -1;
    }
    arp_hdr_t *arp = (arp_hdr_t *)packet_data(pkt);

    uint16_t op = ntohs(arp->ar_op);
    uint16_t pro = ntohs(arp->ar_pro);

    if (pro != ETH_P_IP || arp->ar_hln != ETH_ALEN || arp->ar_pln != 4) {
        return -1; // Unsupported
    }

    ipv4_addr_t spa, tpa;
    memcpy(&spa.addr, arp->ar_spa, 4);
    memcpy(&tpa.addr, arp->ar_tpa, 4);
    mac_addr_t sha;
    memcpy(sha.addr, arp->ar_sha, ETH_ALEN);

    /* Process incoming ARP packet, update cache if relevant */
    arp_update_cache(spa, &sha);

    if (op == ARPOP_REQUEST && tpa.addr == ni->ip_addr.addr) {
        /* They are asking for our MAC, send reply */
        packet_t *reply = packet_alloc(sizeof(arp_hdr_t) + sizeof(eth_hdr_t));
        if (reply) {
            packet_reserve(reply, sizeof(eth_hdr_t));
            arp_hdr_t *rep = (arp_hdr_t *)packet_append(reply, sizeof(arp_hdr_t));
            rep->ar_hrd = htons(ARPHRD_ETHER);
            rep->ar_pro = htons(ETH_P_IP);
            rep->ar_hln = ETH_ALEN;
            rep->ar_pln = 4;
            rep->ar_op  = htons(ARPOP_REPLY);

            memcpy(rep->ar_sha, ni->mac.addr, ETH_ALEN);
            memcpy(rep->ar_spa, &ni->ip_addr.addr, 4);
            memcpy(rep->ar_tha, sha.addr, ETH_ALEN);
            memcpy(rep->ar_tpa, &spa.addr, 4);

            eth_build_hdr(reply, &sha, &ni->mac, ETH_P_ARP);

            if (netif_tx(ni, reply) != 0) {
                packet_free(reply);
            }
        }
    }

    return 0;
}
