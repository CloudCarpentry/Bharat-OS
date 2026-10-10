#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

#include "ipv4.h"
#include "tcp.h"
#include "netbuf.h"
#include "checksum.h"
#include "socket_table.h"

// Mock state
static int mock_ipv4_tx_return_val = 0;
static int mock_ipv4_tx_called = 0;
static uint32_t mock_ipv4_tx_dst_ip = 0;
static uint8_t mock_ipv4_tx_protocol = 0;
static uint8_t mock_ipv4_tx_captured_packet[NETBUF_MAX_SIZE];
static uint16_t mock_ipv4_tx_captured_len = 0;
static uint32_t mock_ipv4_get_source_ip_return = 0;

// Mocks for dependencies
uint32_t ipv4_get_source_ip(uint32_t dst_ip) {
    return mock_ipv4_get_source_ip_return;
}

int ipv4_tx(netbuf_t *nb, uint32_t dst_ip, uint8_t protocol) {
    mock_ipv4_tx_called++;
    mock_ipv4_tx_dst_ip = dst_ip;
    mock_ipv4_tx_protocol = protocol;

    mock_ipv4_tx_captured_len = netbuf_len(nb);
    if (mock_ipv4_tx_captured_len <= NETBUF_MAX_SIZE) {
        memcpy(mock_ipv4_tx_captured_packet, netbuf_data(nb), mock_ipv4_tx_captured_len);
    }

    return mock_ipv4_tx_return_val;
}

// Ensure these exist for linking, though not strictly needed here
int ipv4_rx(netbuf_t *nb) { return -1; }
int ipv4_set_local_ip(uint32_t ip) { return -1; }
uint32_t ipv4_get_local_ip(void) { return 0; }
uint32_t ipv4_get_loopback_ip(void) { return 0; }
int ipv4_is_local_address(uint32_t ip) { return 0; }


void reset_mocks() {
    mock_ipv4_tx_return_val = 0;
    mock_ipv4_tx_called = 0;
    mock_ipv4_tx_dst_ip = 0;
    mock_ipv4_tx_protocol = 0;
    mock_ipv4_tx_captured_len = 0;
    memset(mock_ipv4_tx_captured_packet, 0, sizeof(mock_ipv4_tx_captured_packet));
    mock_ipv4_get_source_ip_return = IPV4_ADDR(192, 168, 1, 100);
}

void test_tcp_tx_valid() {
    reset_mocks();
    socket_table_init();

    int sock = socket_create();
    socket_t *s = socket_get(sock);
    socket_bind(sock, IPV4_ADDR(192, 168, 1, 100), 12345);
    s->tcp_state = TCP_STATE_ESTABLISHED;
    s->tcp_seq = 1000;
    s->tcp_ack = 2000;
    s->tcp_window = 8192;

    uint32_t dst_ip = IPV4_ADDR(192, 168, 1, 101);
    uint16_t dst_port = 80;
    uint8_t payload[] = "Hello";

    int res = tcp_tx(sock, dst_ip, dst_port, payload, sizeof(payload));

    assert(res == 0);
    assert(mock_ipv4_tx_called == 1);
    assert(mock_ipv4_tx_dst_ip == dst_ip);
    assert(mock_ipv4_tx_protocol == IPPROTO_TCP);
    assert(mock_ipv4_tx_captured_len == sizeof(tcphdr_t) + sizeof(payload));

    // Check TCP header bytes
    tcphdr_t *tcph = (tcphdr_t *)mock_ipv4_tx_captured_packet;
    assert(bnet_ntohs(tcph->source) == 12345);
    assert(bnet_ntohs(tcph->dest) == 80);
    assert(bnet_ntohl(tcph->seq) == 1000);
    assert(bnet_ntohl(tcph->ack_seq) == 2000);
    assert(tcph->doff == 5);
    assert(tcph->psh == 1);
    assert(tcph->ack == 1);
    assert(bnet_ntohs(tcph->window) == 8192);

    // Independently verify checksum
    uint16_t sent_csum = tcph->check;
    tcph->check = 0;
    uint16_t calc_csum = net_csum_tcp_ipv4(tcph, mock_ipv4_tx_captured_len, IPV4_ADDR(192, 168, 1, 100), dst_ip);
    assert(sent_csum == calc_csum);

    // Verify sequence updated on success
    assert(s->tcp_seq == 1000 + sizeof(payload));

    printf("test_tcp_tx_valid passed\n");
}

void test_tcp_tx_unsupported_state() {
    reset_mocks();
    socket_table_init();
    int sock = socket_create();
    socket_t *s = socket_get(sock);
    s->tcp_state = TCP_STATE_CLOSED;

    int res = tcp_tx(sock, IPV4_ADDR(10, 0, 0, 1), 80, (const uint8_t *)"test", 4);
    assert(res == -1);
    assert(mock_ipv4_tx_called == 0);
    printf("test_tcp_tx_unsupported_state passed\n");
}

void test_tcp_tx_invalid_socket() {
    reset_mocks();
    int res = tcp_tx(999, IPV4_ADDR(10, 0, 0, 1), 80, (const uint8_t *)"test", 4);
    assert(res == -1);
    assert(mock_ipv4_tx_called == 0);
    printf("test_tcp_tx_invalid_socket passed\n");
}

void test_tcp_tx_ipv4_fail() {
    reset_mocks();
    socket_table_init();
    int sock = socket_create();
    socket_t *s = socket_get(sock);
    socket_bind(sock, IPV4_ADDR(192, 168, 1, 100), 12345);
    s->tcp_state = TCP_STATE_ESTABLISHED;
    s->tcp_seq = 1000;

    mock_ipv4_tx_return_val = -1; // Inject failure

    int res = tcp_tx(sock, IPV4_ADDR(10, 0, 0, 1), 80, (const uint8_t *)"test", 4);
    assert(res == -1);
    assert(mock_ipv4_tx_called == 1);

    // Verify sequence DID NOT advance
    assert(s->tcp_seq == 1000);
    printf("test_tcp_tx_ipv4_fail passed\n");
}

void test_tcp_tx_zero_length() {
    reset_mocks();
    socket_table_init();
    int sock = socket_create();
    socket_t *s = socket_get(sock);
    socket_bind(sock, IPV4_ADDR(192, 168, 1, 100), 12345);
    s->tcp_state = TCP_STATE_ESTABLISHED;
    s->tcp_seq = 1000;

    int res = tcp_tx(sock, IPV4_ADDR(10, 0, 0, 1), 80, NULL, 0);
    assert(res == 0);
    assert(mock_ipv4_tx_called == 1);
    assert(mock_ipv4_tx_captured_len == sizeof(tcphdr_t));

    tcphdr_t *tcph = (tcphdr_t *)mock_ipv4_tx_captured_packet;
    assert(tcph->psh == 0);

    assert(s->tcp_seq == 1000); // 0 bytes sent, 0 seq advancement
    printf("test_tcp_tx_zero_length passed\n");
}

void test_tcp_tx_capacity_fail() {
    reset_mocks();
    socket_table_init();
    int sock = socket_create();
    socket_t *s = socket_get(sock);
    s->tcp_state = TCP_STATE_ESTABLISHED;

    int res = tcp_tx(sock, IPV4_ADDR(10, 0, 0, 1), 80, (const uint8_t *)"test", 65535);
    assert(res == -1);
    assert(mock_ipv4_tx_called == 0);
    printf("test_tcp_tx_capacity_fail passed\n");
}


int main() {
    test_tcp_tx_valid();
    test_tcp_tx_unsupported_state();
    test_tcp_tx_invalid_socket();
    test_tcp_tx_ipv4_fail();
    test_tcp_tx_zero_length();
    test_tcp_tx_capacity_fail();

    printf("All mocked TCP TX tests passed!\n");
    return 0;
}
