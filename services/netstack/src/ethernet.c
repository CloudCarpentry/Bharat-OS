#include "ethernet.h"
#include "checksum.h"
#include "arp.h"
#include "ipv4.h"
//#include <stdio.h>

// Assuming basic interface config locally for Phase 2:
uint8_t local_mac[ETH_ALEN] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

int ethernet_rx(netbuf_t *nb) {
    if (netbuf_len(nb) < sizeof(ethhdr_t)) {
        return -1; // Drop runt frame
    }

    ethhdr_t *eth = (ethhdr_t *)netbuf_data(nb);
    uint16_t protocol = bnet_ntohs(eth->h_proto);

    // Filter out packets not for us (and not broadcast/multicast)
    int is_bcast = 1;
    for (int i = 0; i < ETH_ALEN; i++) {
        if (eth->h_dest[i] != 0xFF) {
            is_bcast = 0;
            break;
        }
    }

    int is_me = 1;
    for (int i = 0; i < ETH_ALEN; i++) {
        if (eth->h_dest[i] != local_mac[i]) {
            is_me = 0;
            break;
        }
    }

    if (!is_bcast && !is_me) {
        return 0; // Ignore silently
    }

    netbuf_pull(nb, sizeof(ethhdr_t));

    switch (protocol) {
        case ETH_P_ARP:
            return arp_rx(nb);
        case ETH_P_IP:
            return ipv4_rx(nb);
        case ETH_P_IPV6:
            // Phase 2: IPv6 not supported yet. Ignore.
            return 0;
        default:
            return -1; // Unknown protocol
    }
}

int ethernet_tx(netbuf_t *nb, const uint8_t *dest_mac, uint16_t protocol) {
    ethhdr_t *eth = (ethhdr_t *)netbuf_push(nb, sizeof(ethhdr_t));
    if (!eth) return -1; // Headroom exhausted

    eth_addr_copy(eth->h_dest, dest_mac);
    eth_addr_copy(eth->h_source, local_mac);
    eth->h_proto = bnet_htons(protocol);

    // This is the point where the frame would be handed off to the driver ring.
    // In Phase 2, this generic hook will be intercepted for loopback or mocked virtio.
    extern int virtio_adapter_tx(netbuf_t *nb);
    return virtio_adapter_tx(nb);
}
