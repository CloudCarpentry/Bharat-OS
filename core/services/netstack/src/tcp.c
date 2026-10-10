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
    if (tcph->doff < 5) {
        return -1; // Invalid data offset (header smaller than 20 bytes)
    }

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

    // Reject unsupported states
    if (sock->tcp_state != TCP_STATE_ESTABLISHED) {
        return -1;
    }

    // Integer overflow protection for netbuf capacity
    uint32_t total_len = sizeof(tcphdr_t) + (uint32_t)len;
    if (total_len > NETBUF_MAX_SIZE || total_len > 0xFFFF) {
        return -1;
    }

    netbuf_t nb;
    netbuf_init(&nb);

    // Append payload if any
    if (len > 0) {
        uint8_t *payload = netbuf_put(&nb, len);
        if (!payload) return -1;
        memcpy(payload, data, len);
    }

    tcphdr_t *tcph = (tcphdr_t *)netbuf_push(&nb, sizeof(tcphdr_t));
    if (!tcph) return -1;

    // Clear the header to zero out padding and bits we don't set explicitly
    memset(tcph, 0, sizeof(tcphdr_t));

    tcph->source = bnet_htons(sock->local_port);
    tcph->dest = bnet_htons(dst_port);
    tcph->seq = bnet_htonl(sock->tcp_seq);
    tcph->ack_seq = bnet_htonl(sock->tcp_ack);
    tcph->doff = 5; // 20 bytes minimum header size (5 words)
    tcph->psh = (len > 0) ? 1 : 0;
    tcph->ack = 1; // Basic phase 1 assumption: always ACK in established state
    tcph->window = bnet_htons(sock->tcp_window);
    tcph->check = 0;
    tcph->urg_ptr = 0;

    // Source IP resolution
    uint32_t src_ip = sock->local_ip;
    if (src_ip == SOCK_ANY_IP) {
        src_ip = ipv4_get_source_ip(dst_ip);
    }
    if (src_ip == 0) {
        return -1;
    }

    tcph->check = net_csum_tcp_ipv4(tcph, total_len, src_ip, dst_ip);

    int res = ipv4_tx(&nb, dst_ip, IPPROTO_TCP);
    if (res == 0) {
        // Advance sequence number
        sock->tcp_seq += len;
    }

    return res;
}
