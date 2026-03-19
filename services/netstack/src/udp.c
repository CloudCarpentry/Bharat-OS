#include "udp.h"
#include "ipv4.h"
#include "icmp.h"
#include "checksum.h"
#include "socket_table.h"
#include <stdio.h>

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
        udph->check = 0;
        uint32_t sum = net_pseudo_checksum(src_ip, dst_ip, IPPROTO_UDP, udp_len);

        const uint16_t *buf = (const uint16_t *)udph;
        size_t len = udp_len;
        while (len > 1) {
            sum += *buf++;
            len -= 2;
        }
        if (len == 1) {
            uint16_t last_byte = 0;
            *(uint8_t *)&last_byte = *(const uint8_t *)buf;
            sum += last_byte;
        }
        while (sum >> 16) {
            sum = (sum & 0xffff) + (sum >> 16);
        }

        if (orig_check != (uint16_t)~sum) {
            udph->check = orig_check;
            return -1; // Checksum failed
        }
        udph->check = orig_check;
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

    // Use default IP if we don't have a specific local IP bound, but the local IP is
    // ultimately decided by IPv4 routing.
    // We'll calculate checksum in IPv4 later, or we can calculate it here by predicting
    // the source IP. In Phase 2, we know the source IP based on the destination.
    uint32_t src_ip = 0x0A01A8C0; // 192.168.1.10
    if ((dst_ip & 0xFF) == 127) {
        src_ip = 0x0100007F; // 127.0.0.1
    }

    uint32_t sum = net_pseudo_checksum(src_ip, dst_ip, IPPROTO_UDP, udp_len);

    const uint16_t *buf = (const uint16_t *)udph;
    size_t check_len = udp_len;
    while (check_len > 1) {
        sum += *buf++;
        check_len -= 2;
    }
    if (check_len == 1) {
        uint16_t last_byte = 0;
        *(uint8_t *)&last_byte = *(const uint8_t *)buf;
        sum += last_byte;
    }
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    udph->check = (uint16_t)~sum;
    if (udph->check == 0) udph->check = 0xFFFF; // UDP specific rule

    return ipv4_tx(&nb, dst_ip, IPPROTO_UDP);
}
