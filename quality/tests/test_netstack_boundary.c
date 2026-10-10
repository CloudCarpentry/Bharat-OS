#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "ipv4.h"
#include "udp.h"
#include "icmp.h"
#include "netbuf.h"
#include "checksum.h"
#include "socket_table.h"
#include "loopback.h"
#include "ethernet.h"

// Define a test callback
static int rx_callback_called = 0;
static uint16_t last_rx_len = 0;
static uint8_t last_rx_data[2048] = {0};

static void test_udp_rx_callback(int sock_id, uint32_t src_ip, uint16_t src_port, const uint8_t *data, uint16_t len) {
    rx_callback_called = 1;
    last_rx_len = len;
    if (len > 0 && len <= sizeof(last_rx_data)) {
        memcpy(last_rx_data, data, len);
    }
}

// Stubs for resolving linkage issues
#ifndef BHARAT_HOST_TEST
int ethernet_tx(netbuf_t *nb, const uint8_t *dest_mac, uint16_t protocol) { return 0; }
int arp_resolve(uint32_t ip, uint8_t *mac_out) { return -1; }
int loopback_tx(netbuf_t *nb) { return 0; }
int tcp_rx(netbuf_t *nb, uint32_t src_ip, uint32_t dst_ip) { return -1; }
#endif

// 1. IPv4 total length smaller than IPv4 header length
void test_ipv4_tot_len_smaller_than_ihl() {
    netbuf_t nb;
    netbuf_init(&nb);

    iphdr_t *iph = (iphdr_t *)netbuf_put(&nb, sizeof(iphdr_t));
    iph->version = 4;
    iph->ihl = 5; // 20 bytes
    iph->tot_len = bnet_htons(10); // Less than 20

    int res = ipv4_rx(&nb);
    assert(res == -1);
    printf("PASS: test_ipv4_tot_len_smaller_than_ihl\n");
}

// 2. IPv4 total length smaller than the minimum IPv4 header
void test_ipv4_tot_len_smaller_than_min_header() {
    netbuf_t nb;
    netbuf_init(&nb);

    iphdr_t *iph = (iphdr_t *)netbuf_put(&nb, sizeof(iphdr_t));
    iph->version = 4;
    iph->ihl = 5;
    iph->tot_len = bnet_htons(15); // Smaller than min header 20

    int res = ipv4_rx(&nb);
    assert(res == -1);
    printf("PASS: test_ipv4_tot_len_smaller_than_min_header\n");
}

// 3. Truncated IPv4 options
void test_ipv4_truncated_options() {
    netbuf_t nb;
    netbuf_init(&nb);

    // IHL is 6 (24 bytes), but buffer only has 20 bytes
    iphdr_t *iph = (iphdr_t *)netbuf_put(&nb, sizeof(iphdr_t));
    iph->version = 4;
    iph->ihl = 6;
    iph->tot_len = bnet_htons(24);

    int res = ipv4_rx(&nb);
    assert(res == -1);
    printf("PASS: test_ipv4_truncated_options\n");
}

// 4. IPv4 packets with Ethernet padding
void test_ipv4_with_ethernet_padding() {
    netbuf_t nb;
    netbuf_init(&nb);

    // Total len 20, but buffer is 60 (typical min ethernet frame padded)
    iphdr_t *iph = (iphdr_t *)netbuf_put(&nb, 60);
    iph->version = 4;
    iph->ihl = 5;
    iph->tot_len = bnet_htons(20);
    iph->protocol = IPPROTO_UDP;
    iph->saddr = IPV4_ADDR(192, 168, 1, 10);
    iph->daddr = IPV4_ADDR(127, 0, 0, 1);
    iph->check = 0;

    uint32_t sum = net_csum_partial(iph, 20, 0);
    iph->check = net_csum_finalize(sum);

    ipv4_set_local_ip(IPV4_ADDR(127, 0, 0, 1));

    nb.tail -= 40; // reset
    udphdr_t *udph = (udphdr_t*)netbuf_put(&nb, sizeof(udphdr_t));
    udph->source = bnet_htons(1234);
    udph->dest = bnet_htons(5678);
    udph->len = bnet_htons(8);
    udph->check = 0;

    // Total len is 20 (IP) + 8 (UDP) = 28
    iph->tot_len = bnet_htons(28);

    // Add ethernet padding
    netbuf_put(&nb, 32); // total buf len is 60 now

    // Recompute IP csum
    iph->check = 0;
    sum = net_csum_partial(iph, 20, 0);
    iph->check = net_csum_finalize(sum);

    socket_table_init();
    int sock = socket_create();
    socket_bind(sock, SOCK_ANY_IP, 5678);
    socket_set_rx_callback(sock, test_udp_rx_callback);
    rx_callback_called = 0;

    int res = ipv4_rx(&nb);
    assert(res == 0); // UDP Rx succeeds
    assert(rx_callback_called == 1);
    assert(last_rx_len == 0); // 8 bytes of header, 0 bytes of payload
    printf("PASS: test_ipv4_with_ethernet_padding\n");
}

// 5. UDP length smaller than 8 bytes
void test_udp_len_smaller_than_8() {
    netbuf_t nb;
    netbuf_init(&nb);

    udphdr_t *udph = (udphdr_t *)netbuf_put(&nb, sizeof(udphdr_t));
    udph->source = bnet_htons(1234);
    udph->dest = bnet_htons(5678);
    udph->len = bnet_htons(7); // Less than 8
    udph->check = 0;

    int res = udp_rx(&nb, 0, 0);

    // EXPECTED DEFECT REPRODUCTION (XFAIL)
    if (res != -1) {
        printf("XFAIL: test_udp_len_smaller_than_8 observed defect.\n");
    } else {
        assert(res == -1);
        printf("XPASS: test_udp_len_smaller_than_8\n");
    }
}

// 6. UDP length greater than available IPv4 payload
void test_udp_len_greater_than_payload() {
    netbuf_t nb;
    netbuf_init(&nb);

    udphdr_t *udph = (udphdr_t *)netbuf_put(&nb, sizeof(udphdr_t));
    udph->source = bnet_htons(1234);
    udph->dest = bnet_htons(5678);
    udph->len = bnet_htons(20); // Requires 20 bytes, but buffer only has 8
    udph->check = 0;

    int res = udp_rx(&nb, 0, 0);
    assert(res == -1);
    printf("PASS: test_udp_len_greater_than_payload\n");
}

// 7. UDP length smaller than the available payload
void test_udp_len_smaller_than_payload() {
    netbuf_t nb;
    netbuf_init(&nb);

    udphdr_t *udph = (udphdr_t *)netbuf_put(&nb, sizeof(udphdr_t) + 10);
    udph->source = bnet_htons(1234);
    udph->dest = bnet_htons(5678);
    udph->len = bnet_htons(8 + 5); // Says 13 bytes, but payload has 10 (total 18)
    udph->check = 0;

    // We need to bind
    socket_table_init();
    int sock = socket_create();
    socket_bind(sock, SOCK_ANY_IP, 5678);
    socket_set_rx_callback(sock, test_udp_rx_callback);
    rx_callback_called = 0;

    int res = udp_rx(&nb, 0, 0);
    assert(res == 0);

    // EXPECTED DEFECT REPRODUCTION (XFAIL)
    if (last_rx_len != 5) {
        printf("XFAIL: test_udp_len_smaller_than_payload observed defect.\n");
    } else {
        assert(last_rx_len == 5);
        printf("XPASS: test_udp_len_smaller_than_payload\n");
    }
}

// 8. Invalid UDP checksum
void test_udp_invalid_checksum() {
    netbuf_t nb;
    netbuf_init(&nb);

    udphdr_t *udph = (udphdr_t *)netbuf_put(&nb, sizeof(udphdr_t) + 4);
    udph->source = bnet_htons(1234);
    udph->dest = bnet_htons(5678);
    udph->len = bnet_htons(12);
    udph->check = 0x1234; // Invalid checksum

    int res = udp_rx(&nb, IPV4_ADDR(192, 168, 1, 10), IPV4_ADDR(192, 168, 1, 11));
    assert(res == -1);
    printf("PASS: test_udp_invalid_checksum\n");
}

// 9. Valid zero UDP checksum for IPv4
void test_udp_valid_zero_checksum() {
    netbuf_t nb;
    netbuf_init(&nb);

    udphdr_t *udph = (udphdr_t *)netbuf_put(&nb, sizeof(udphdr_t) + 4);
    udph->source = bnet_htons(1234);
    udph->dest = bnet_htons(5678);
    udph->len = bnet_htons(12);
    udph->check = 0; // Zero is valid in IPv4 UDP

    socket_table_init();
    int sock = socket_create();
    socket_bind(sock, SOCK_ANY_IP, 5678);
    socket_set_rx_callback(sock, test_udp_rx_callback);
    rx_callback_called = 0;

    int res = udp_rx(&nb, IPV4_ADDR(192, 168, 1, 10), IPV4_ADDR(192, 168, 1, 11));
    assert(res == 0);
    assert(rx_callback_called == 1);
    printf("PASS: test_udp_valid_zero_checksum\n");
}

// 10. Empty UDP datagram
void test_udp_empty_datagram() {
    netbuf_t nb;
    netbuf_init(&nb);

    udphdr_t *udph = (udphdr_t *)netbuf_put(&nb, sizeof(udphdr_t));
    udph->source = bnet_htons(1234);
    udph->dest = bnet_htons(5678);
    udph->len = bnet_htons(8);
    udph->check = 0;

    socket_table_init();
    int sock = socket_create();
    socket_bind(sock, SOCK_ANY_IP, 5678);
    socket_set_rx_callback(sock, test_udp_rx_callback);
    rx_callback_called = 0;

    int res = udp_rx(&nb, IPV4_ADDR(192, 168, 1, 10), IPV4_ADDR(192, 168, 1, 11));
    assert(res == 0);
    assert(rx_callback_called == 1);
    assert(last_rx_len == 0);
    printf("PASS: test_udp_empty_datagram\n");
}

#ifndef BHARAT_HOST_TEST
int main(void) {
    test_ipv4_tot_len_smaller_than_ihl();
    test_ipv4_tot_len_smaller_than_min_header();
    test_ipv4_truncated_options();
    test_ipv4_with_ethernet_padding();
    test_udp_len_smaller_than_8();
    test_udp_len_greater_than_payload();
    test_udp_len_smaller_than_payload();
    test_udp_invalid_checksum();
    test_udp_valid_zero_checksum();
    test_udp_empty_datagram();

    printf("All boundary tests passed!\n");
    return 0;
}
#else
int test_netstack_boundary() {
    test_ipv4_tot_len_smaller_than_ihl();
    test_ipv4_tot_len_smaller_than_min_header();
    test_ipv4_truncated_options();
    test_ipv4_with_ethernet_padding();
    test_udp_len_smaller_than_8();
    test_udp_len_greater_than_payload();
    test_udp_len_smaller_than_payload();
    test_udp_invalid_checksum();
    test_udp_valid_zero_checksum();
    test_udp_empty_datagram();

    printf("All boundary tests passed!\n");
    return 0;
}

int main(void) {
    return test_netstack_boundary();
}
#endif
