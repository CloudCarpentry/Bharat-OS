#include "net/dhcp_client.h"
#include <stdio.h>

static dhcp_client_t eth0_dhcp;

void netstack_dhcp_init(netif_t *ni) {
    if (!ni) return;
    dhcp_client_init(&eth0_dhcp, ni);
    dhcp_client_start(&eth0_dhcp);
    printf("[netstack_dhcp] Initialized DHCP for %s\n", ni->name);
}

void netstack_dhcp_tick(void) {
    dhcp_client_tick(&eth0_dhcp);
}
