#include "udp.h"
#include "ipv4.h"
#include "icmp.h"
#include "checksum.h"
#include "socket_table.h"
#include <bharat/runtime/freestanding_string.h>
//#include <stdio.h>

int udp_rx(netbuf_t *nb, uint32_t src_ip, uint32_t dst_ip) {
    if (netbuf_len(nb) < sizeof(udphdr_t)) {
        return -1; // Runt packet
    }

    udphdr_t *udph = (udphdr_t *)netbuf_data(nb);
    uint16_t udp_len = bnet_ntohs(udph->len);

    if (netbuf_len(nb) < udp_len) {
        return -1; // Truncated UDP packet
    }

    uint16_t orig_check = udph->check;
    if (orig_check != 0) {
        if (nb->flags & NETBUF_F_RX_L4_CSUM_BAD) {
            return -1;
        }
        if (!(nb->flags & NETBUF_F_RX_L4_CSUM_OK)) {
            udph->check = 0;
            uint16_t calc_check = net_csum_udp_ipv4(udph, udp_len, src_ip, dst_ip);
            if (orig_check != calc_check) {
                udph->check = orig_check;
                return -1; // Checksum failed
            }
            udph->check = orig_check;
        }
    }

    uint16_t src_port = bnet_ntohs(udph->source);
    uint16_t dst_port = bnet_ntohs(udph->dest);

    netbuf_pull(nb, sizeof(udphdr_t));

    socket_t *sock = socket_lookup(dst_ip, dst_port);
    if (!sock) {
        // Unreachable port
        // Re-push UDP header for ICMP payload
        netbuf_push(nb, sizeof(udphdr_t));
        return icmp_send_unreachable(nb, src_ip, dst_ip, ICMP_UNREACH_PORT);
    }

    if (sock->rx_callback) {
        sock->rx_callback(sock->id, src_ip, src_port, netbuf_data(nb), netbuf_len(nb));
    }

    return 0;
}

int udp_tx(int sock_id, uint32_t dst_ip, uint16_t dst_port, const uint8_t *data, uint16_t len) {
    socket_t *sock = socket_get(sock_id);
    if (!sock) return -1;

    netbuf_t nb;
    netbuf_init(&nb);

    uint8_t *payload = netbuf_put(&nb, len);
    if (!payload) return -1;
    memcpy(payload, data, len);

    uint16_t udp_len = sizeof(udphdr_t) + len;

    udphdr_t *udph = (udphdr_t *)netbuf_push(&nb, sizeof(udphdr_t));
    if (!udph) return -1;

    udph->source = bnet_htons(sock->local_port);
    udph->dest = bnet_htons(dst_port);
    udph->len = bnet_htons(udp_len);
    udph->check = 0;

    // Use the socket's bound local IP if available, otherwise determine it based on routing.
    uint32_t src_ip = sock->local_ip;
    if (src_ip == SOCK_ANY_IP) {
        src_ip = ipv4_get_source_ip(dst_ip);
    }

    // If routing failed to find a valid source IP (unconfigured non-loopback), fail early.
    if (src_ip == 0) {
        return -1;
    }

    udph->check = net_csum_udp_ipv4(udph, udp_len, src_ip, dst_ip);

    return ipv4_tx(&nb, dst_ip, IPPROTO_UDP);
}
