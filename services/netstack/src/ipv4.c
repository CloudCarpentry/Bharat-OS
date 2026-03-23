#include "ipv4.h"
#include "icmp.h"
#include "tcp.h"
#include "udp.h"
#include "ethernet.h"
#include "checksum.h"
#include "arp.h"
#include "loopback.h"
#include <bharat/runtime/freestanding_string.h>
//#include <stdio.h>

static uint32_t local_ip = 0;          /* unconfigured by default */
static const uint32_t loopback_ip = 0x0100007F; /* 127.0.0.1 */
static uint16_t ip_id_counter = 0;

static int ipv4_is_broadcast(uint32_t ip) {
    return ip == 0xFFFFFFFFu;
}

int ipv4_set_local_ip(uint32_t ip) {
    /* allow 0 to explicitly clear configuration */
    if (ip == 0) {
        local_ip = 0;
        return 0;
    }

    /* reject obviously invalid assignments */
    if (ip == loopback_ip || ipv4_is_broadcast(ip)) {
        return -1;
    }

    local_ip = ip;
    return 0;
}

uint32_t ipv4_get_local_ip(void) {
    return local_ip;
}

uint32_t ipv4_get_loopback_ip(void) {
    return loopback_ip;
}

int ipv4_is_local_address(uint32_t ip) {
    if (ip == loopback_ip || ipv4_is_broadcast(ip)) {
        return 1;
    }

    return (local_ip != 0 && ip == local_ip);
}

int ipv4_rx(netbuf_t *nb) {
    if (netbuf_len(nb) < sizeof(iphdr_t)) {
        return -1; // Runt packet
    }

    iphdr_t *iph = (iphdr_t *)netbuf_data(nb);

    if (iph->version != 4 || iph->ihl < 5) {
        return -1; // Invalid version or header length
    }

    uint16_t header_len = iph->ihl * 4;
    if (netbuf_len(nb) < header_len) {
        return -1; // Truncated header
    }

    // Verify Checksum
    // Skip software validation if hardware has already verified the checksum.
    if (nb->flags & NETBUF_F_RX_IP_CSUM_BAD) {
        return -1;
    }
    if (!(nb->flags & NETBUF_F_RX_IP_CSUM_OK)) {
        uint16_t orig_check = iph->check;
        iph->check = 0;
        uint32_t sum = net_csum_partial(iph, header_len, 0);
        if (orig_check != net_csum_finalize(sum)) {
            iph->check = orig_check; // Restore
            return -1; // Checksum failed
        }
        iph->check = orig_check;
    }

    uint32_t dst_ip = iph->daddr;
    uint32_t src_ip = iph->saddr;
    uint8_t protocol = iph->protocol;
    uint16_t total_len = bnet_ntohs(iph->tot_len);

    if (total_len > netbuf_len(nb)) {
        return -1; // Fragmented or malformed
    }

    // Trim trailing Ethernet padding if any
    if (total_len < netbuf_len(nb)) {
        nb->tail -= (netbuf_len(nb) - total_len);
    }

    // Address check (Local or Loopback)
    if (!ipv4_is_local_address(dst_ip)) {
        return 0; // Not for us, drop (no forwarding in Phase 2)
    }

    netbuf_pull(nb, header_len);

    switch (protocol) {
        case IPPROTO_ICMP:
            return icmp_rx(nb, src_ip, dst_ip);
        case IPPROTO_TCP:
            return tcp_rx(nb, src_ip, dst_ip);
        case IPPROTO_UDP:
            return udp_rx(nb, src_ip, dst_ip);
        default:
            // Send ICMP Unreachable (Protocol)
            return icmp_send_unreachable(nb, src_ip, dst_ip, ICMP_UNREACH_PROTOCOL);
    }
}

uint32_t ipv4_get_source_ip(uint32_t dst_ip) {
    if ((dst_ip & 0xFF) == 127) { // 127.x.x.x loopback
        return loopback_ip;
    }

    /* fail closed: no non-loopback transmit without configuration */
    if (local_ip == 0) {
        return 0;
    }
    return local_ip;
}

int ipv4_tx(netbuf_t *nb, uint32_t dst_ip, uint8_t protocol) {
    if (!nb) {
        return -1;
    }

    uint32_t src_ip = ipv4_get_source_ip(dst_ip);
    if (src_ip == 0) {
        return -1; // Fail closed if unconfigured non-loopback
    }

    uint16_t total_len = netbuf_len(nb) + sizeof(iphdr_t);

    iphdr_t *iph = (iphdr_t *)netbuf_push(nb, sizeof(iphdr_t));
    if (!iph) return -1; // Headroom exhausted

    iph->version = 4;
    iph->ihl = 5;
    iph->tos = 0;
    iph->tot_len = bnet_htons(total_len);
    iph->id = bnet_htons(ip_id_counter++);
    iph->frag_off = bnet_htons(0x4000); // Don't fragment
    iph->ttl = 64;
    iph->protocol = protocol;
    iph->saddr = src_ip;
    iph->daddr = dst_ip;

    iph->check = 0;

    // Support TX checksum offload if requested
    if (nb->flags & NETBUF_F_TX_IP_CSUM_PARTIAL) {
        // Leave checksum as 0, hardware will fill it in
        // In reality, some drivers may require pseudo-header checksums,
        // but for IP headers, 0 is typically sufficient.
    } else {
        uint32_t sum = net_csum_partial(iph, sizeof(iphdr_t), 0);
        iph->check = net_csum_finalize(sum);
        nb->flags |= NETBUF_F_TX_IP_CSUM_DONE;
    }

    // Routing decision
    if (src_ip == loopback_ip || dst_ip == loopback_ip || (local_ip != 0 && dst_ip == local_ip)) {
        // Send to loopback interface
        return loopback_tx(nb);
    } else {
        // Send to Ethernet (requires ARP)
        uint8_t dest_mac[ETH_ALEN];
        if (dst_ip == 0xFFFFFFFFu) {
            memset(dest_mac, 0xFF, ETH_ALEN);
        } else {
            if (arp_resolve(dst_ip, dest_mac) < 0) {
                // ARP resolution pending or failed
                return -1;
            }
        }
        return ethernet_tx(nb, dest_mac, ETH_P_IP);
    }
}
