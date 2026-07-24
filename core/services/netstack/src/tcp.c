#include "tcp.h"
#include "ipv4.h"
#include "checksum.h"
#include "socket_table.h"

/* Forward declare string operations for freestanding environment */
#include <bharat/runtime/freestanding_string.h>

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
    if (nb->flags & NETBUF_F_RX_L4_CSUM_BAD) {
        return -1;
    }
    if (!(nb->flags & NETBUF_F_RX_L4_CSUM_OK)) {
        uint16_t orig_check = tcph->check;
        tcph->check = 0;

        uint16_t calc_check = net_csum_tcp_ipv4(tcph, netbuf_len(nb), src_ip, dst_ip);

        if (orig_check != calc_check) {
            tcph->check = orig_check;
            return -1; // Checksum failed
        }
        tcph->check = orig_check;
    }

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
