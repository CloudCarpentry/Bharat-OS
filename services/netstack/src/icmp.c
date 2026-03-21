#include "icmp.h"
#include "ipv4.h"
#include "checksum.h"
#include <bharat/runtime/freestanding_string.h>
//#include <stdio.h>

int icmp_rx(netbuf_t *nb, uint32_t src_ip, uint32_t dst_ip) {
    if (netbuf_len(nb) < sizeof(icmphdr_t)) {
        return -1; // Runt packet
    }

    icmphdr_t *icmph = (icmphdr_t *)netbuf_data(nb);

    // Skip software checksum verification if hardware already validated it.
    // The NETBUF_F_RX_CSUM_VALID flag applies to the originally received packet.
    if (!(nb->flags & NETBUF_F_RX_CSUM_VALID)) {
        uint16_t orig_check = icmph->checksum;
        icmph->checksum = 0;
        if (orig_check != net_checksum(icmph, netbuf_len(nb))) {
            icmph->checksum = orig_check;
            return -1; // Checksum failed
        }
        icmph->checksum = orig_check;
    }

    if (icmph->type == ICMP_ECHO) {
        // Prepare to send an echo reply. Keep the payload intact.
        netbuf_pull(nb, sizeof(icmphdr_t)); // Hide header momentarily
        return icmp_send_echo_reply(nb, src_ip, dst_ip, icmph->un.echo.id, icmph->un.echo.sequence);
    } else {
        // Ignore other ICMP types for Phase 2
        return 0;
    }
}

int icmp_send_echo_reply(netbuf_t *nb, uint32_t src_ip, uint32_t dst_ip, uint16_t id, uint16_t seq) {
    icmphdr_t *icmph = (icmphdr_t *)netbuf_push(nb, sizeof(icmphdr_t));
    if (!icmph) return -1;

    icmph->type = ICMP_ECHO_REPLY;
    icmph->code = 0;
    icmph->un.echo.id = id;
    icmph->un.echo.sequence = seq;

    icmph->checksum = 0;
    icmph->checksum = net_checksum(icmph, netbuf_len(nb));

    // Send back to the sender
    return ipv4_tx(nb, src_ip, IPPROTO_ICMP);
}

int icmp_send_unreachable(netbuf_t *nb, uint32_t src_ip, uint32_t dst_ip, uint8_t code) {
    // We need to send back the original IP header + 8 bytes of payload
    // Re-expose the IP header
    if (nb->head < sizeof(iphdr_t)) {
        return -1; // Not enough room to construct Unreachable payload
    }

    netbuf_t reply_nb;
    netbuf_init(&reply_nb);

    uint16_t payload_len = sizeof(iphdr_t) + 8; // IP header + 64 bits of original payload
    if (netbuf_len(nb) + sizeof(iphdr_t) < payload_len) {
        payload_len = netbuf_len(nb) + sizeof(iphdr_t); // Short packet
    }

    uint8_t *payload = netbuf_put(&reply_nb, payload_len);
    if (!payload) return -1;

    // Copy original IP header and 8 bytes of payload
    memcpy(payload, netbuf_data(nb) - sizeof(iphdr_t), payload_len);

    icmphdr_t *icmph = (icmphdr_t *)netbuf_push(&reply_nb, sizeof(icmphdr_t));
    if (!icmph) return -1;

    icmph->type = ICMP_UNREACH;
    icmph->code = code;
    icmph->un.gateway = 0; // Unused for these codes

    icmph->checksum = 0;
    icmph->checksum = net_checksum(icmph, netbuf_len(&reply_nb));

    return ipv4_tx(&reply_nb, src_ip, IPPROTO_ICMP);
}
