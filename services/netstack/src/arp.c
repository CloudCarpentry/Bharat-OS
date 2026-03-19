#include "arp.h"
#include "ethernet.h"
#include "checksum.h"
//#include <stdio.h>

// A simple ARP cache for Phase 2: max 16 entries
#define ARP_CACHE_SIZE 16

typedef struct {
    uint32_t ip;
    uint8_t mac[ETH_ALEN];
    uint32_t expires; // Time to live (simple counter/timestamp)
} arp_entry_t;

static arp_entry_t arp_cache[ARP_CACHE_SIZE] = {0};
static int arp_cache_count = 0;

static uint8_t local_ip_arr[4] = {192, 168, 1, 10}; // Phase 2 static IP

static uint32_t get_local_ip() {
    return *(uint32_t *)local_ip_arr;
}

static uint8_t *get_local_mac() {
    extern uint8_t local_mac[ETH_ALEN];
    return local_mac;
}

static void arp_cache_update(uint32_t ip, const uint8_t *mac) {
    // 1. Update existing entry
    for (int i = 0; i < arp_cache_count; i++) {
        if (arp_cache[i].ip == ip) {
            memcpy(arp_cache[i].mac, mac, ETH_ALEN);
            arp_cache[i].expires = 300; // Reset TTL (arbitrary value)
            return;
        }
    }

    // 2. Or add new entry
    if (arp_cache_count < ARP_CACHE_SIZE) {
        arp_cache[arp_cache_count].ip = ip;
        memcpy(arp_cache[arp_cache_count].mac, mac, ETH_ALEN);
        arp_cache[arp_cache_count].expires = 300;
        arp_cache_count++;
    } else {
        // Simple eviction: replace the first entry
        arp_cache[0].ip = ip;
        memcpy(arp_cache[0].mac, mac, ETH_ALEN);
        arp_cache[0].expires = 300;
    }
}

int arp_resolve(uint32_t target_ip, uint8_t *out_mac) {
    for (int i = 0; i < arp_cache_count; i++) {
        if (arp_cache[i].ip == target_ip) {
            memcpy(out_mac, arp_cache[i].mac, ETH_ALEN);
            return 0; // Found in cache
        }
    }

    // Not found in cache. In a real system, we'd queue the packet and send a request.
    // For Phase 2, we return -1 and trigger an ARP request immediately.
    arp_send_request(target_ip);
    return -1; // Blocking lookup not implemented yet; wait for reply.
}

int arp_send_reply(uint32_t target_ip, const uint8_t *target_mac) {
    netbuf_t nb;
    netbuf_init(&nb);

    // Build ARP payload
    arphdr_t *arph = (arphdr_t *)netbuf_put(&nb, sizeof(arphdr_t));
    if (!arph) return -1;

    arph->ar_hrd = bnet_htons(ARP_HRD_ETHER);
    arph->ar_pro = bnet_htons(ARP_PRO_IP);
    arph->ar_hln = ETH_ALEN;
    arph->ar_pln = 4;
    arph->ar_op  = bnet_htons(ARP_OP_REPLY);

    arphdr_payload_t *arp_payload = (arphdr_payload_t *)netbuf_put(&nb, sizeof(arphdr_payload_t));
    if (!arp_payload) return -1;

    memcpy(arp_payload->ar_sha, get_local_mac(), ETH_ALEN);
    arp_payload->ar_sip = get_local_ip();
    memcpy(arp_payload->ar_tha, target_mac, ETH_ALEN);
    arp_payload->ar_tip = target_ip;

    // Send via Ethernet
    return ethernet_tx(&nb, target_mac, ETH_P_ARP);
}

int arp_send_request(uint32_t target_ip) {
    netbuf_t nb;
    netbuf_init(&nb);

    // Build ARP payload
    arphdr_t *arph = (arphdr_t *)netbuf_put(&nb, sizeof(arphdr_t));
    if (!arph) return -1;

    arph->ar_hrd = bnet_htons(ARP_HRD_ETHER);
    arph->ar_pro = bnet_htons(ARP_PRO_IP);
    arph->ar_hln = ETH_ALEN;
    arph->ar_pln = 4;
    arph->ar_op  = bnet_htons(ARP_OP_REQUEST);

    arphdr_payload_t *arp_payload = (arphdr_payload_t *)netbuf_put(&nb, sizeof(arphdr_payload_t));
    if (!arp_payload) return -1;

    memcpy(arp_payload->ar_sha, get_local_mac(), ETH_ALEN);
    arp_payload->ar_sip = get_local_ip();
    memset(arp_payload->ar_tha, 0, ETH_ALEN); // Target MAC unknown
    arp_payload->ar_tip = target_ip;

    uint8_t bcast_mac[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    return ethernet_tx(&nb, bcast_mac, ETH_P_ARP);
}

int arp_rx(netbuf_t *nb) {
    if (netbuf_len(nb) < sizeof(arphdr_t) + sizeof(arphdr_payload_t)) {
        return -1; // Runt packet
    }

    arphdr_t *arph = (arphdr_t *)netbuf_data(nb);
    netbuf_pull(nb, sizeof(arphdr_t));

    if (bnet_ntohs(arph->ar_hrd) != ARP_HRD_ETHER ||
        bnet_ntohs(arph->ar_pro) != ARP_PRO_IP ||
        arph->ar_hln != ETH_ALEN ||
        arph->ar_pln != 4) {
        return -1; // Unsupported hardware/protocol type
    }

    arphdr_payload_t *arp_payload = (arphdr_payload_t *)netbuf_data(nb);
    netbuf_pull(nb, sizeof(arphdr_payload_t));

    uint16_t op = bnet_ntohs(arph->ar_op);

    if (op == ARP_OP_REQUEST) {
        if (arp_payload->ar_tip == get_local_ip()) {
            // It's a request for our IP. Send a reply.
            arp_cache_update(arp_payload->ar_sip, arp_payload->ar_sha); // Opportunistic cache
            return arp_send_reply(arp_payload->ar_sip, arp_payload->ar_sha);
        }
    } else if (op == ARP_OP_REPLY) {
        if (arp_payload->ar_tip == get_local_ip()) {
            // It's a reply addressed to us. Update cache.
            arp_cache_update(arp_payload->ar_sip, arp_payload->ar_sha);
            return 0;
        }
    }

    return -1; // Ignore other ops or replies not for us
}
