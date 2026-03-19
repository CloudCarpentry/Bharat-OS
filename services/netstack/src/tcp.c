#include "tcp.h"
#include "ipv4.h"
#include "checksum.h"
#include "socket_table.h"
#include <string.h>

/* Minimal TCP stack phase 1 - processing incoming segments and basic verification */

int tcp_rx(netbuf_t *nb, uint32_t src_ip, uint32_t dst_ip) {
    if (netbuf_len(nb) < sizeof(tcphdr_t)) {
        return -1; // Runt packet
    }

    tcphdr_t *tcph = (tcphdr_t *)netbuf_data(nb);
    uint16_t header_len = tcph->doff * 4;

    if (netbuf_len(nb) < header_len) {
        return -1; // Truncated header
    }

    // Verify Checksum
    uint16_t orig_check = tcph->check;
    tcph->check = 0;

    uint32_t sum = net_pseudo_checksum(src_ip, dst_ip, IPPROTO_TCP, netbuf_len(nb));

    const uint16_t *buf = (const uint16_t *)netbuf_data(nb);
    size_t len = netbuf_len(nb);
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
        tcph->check = orig_check;
        return -1; // Checksum failed
    }
    tcph->check = orig_check;

    uint16_t src_port = bnet_ntohs(tcph->source);
    uint16_t dst_port = bnet_ntohs(tcph->dest);

    // Look up socket
    socket_t *sock = socket_lookup(dst_ip, dst_port);
    if (!sock) {
        // TCP RST response would go here for closed ports
        return -1;
    }

    // For now, simple logging/callback hook up
    // In future phases: Full TCP state machine handling (SYN, ACK, FIN, window management)

    // Advance beyond TCP header
    netbuf_pull(nb, header_len);

    if (sock->rx_callback && netbuf_len(nb) > 0) {
        // Pass payload to socket callback
        sock->rx_callback(sock->id, src_ip, src_port, netbuf_data(nb), netbuf_len(nb));
    }

    return 0;
}

int tcp_tx(int sock_id, uint32_t dst_ip, uint16_t dst_port, const uint8_t *data, uint16_t len) {
    socket_t *sock = socket_get(sock_id);
    if (!sock) return -1;

    // TCP transmission placeholder.
    // This will involve sequence numbers, window sizes, and state machine updates.
    (void)dst_ip;
    (void)dst_port;
    (void)data;
    (void)len;

    return -1; // Not implemented yet
}
