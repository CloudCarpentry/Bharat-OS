#include "net/netif.h"
#include <stdio.h>

// Dummy port for testing
static int dummy_tx(netif_t *ni, packet_t *pkt) {
    printf("[netstack_port] dummy_tx: sending packet of len %u on %s\n", packet_length(pkt), ni->name);
    packet_free(pkt);
    return 0;
}

static int dummy_up(netif_t *ni) {
    printf("[netstack_port] dummy_up: %s is now up\n", ni->name);
    return 0;
}

static int dummy_down(netif_t *ni) {
    printf("[netstack_port] dummy_down: %s is now down\n", ni->name);
    return 0;
}

static netif_ops_t dummy_ops = {
    .tx = dummy_tx,
    .up = dummy_up,
    .down = dummy_down
};

static netif_t eth0;

void netstack_port_init(void) {
    mac_addr_t mac = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
    netif_init(&eth0, "eth0", &dummy_ops, NULL);
    netif_set_mac(&eth0, &mac);

    ipv4_addr_t ip = { .addr = 0x0100a8c0 }; // 192.168.0.1
    ipv4_addr_t mask = { .addr = 0x00ffffff }; // 255.255.255.0
    ipv4_addr_t gw = { .addr = 0xfe00a8c0 }; // 192.168.0.254
    netif_set_ip(&eth0, ip, mask, gw);

    netif_set_up(&eth0);
    eth0.link_up = true;
    printf("[netstack_port] eth0 initialized\n");
}
