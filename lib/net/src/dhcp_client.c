#include "net/dhcp_client.h"
#include "net/udp.h"
#include <string.h>
#include <stdio.h>

void dhcp_client_init(dhcp_client_t *c, netif_t *ni) {
    memset(c, 0, sizeof(dhcp_client_t));
    c->ni = ni;
    c->state = DHCP_STATE_INIT;
}

int dhcp_client_start(dhcp_client_t *c) {
    if (!c || !c->ni) return -1;
    printf("[dhcp_client] Starting DHCP on %s\n", c->ni->name);
    c->state = DHCP_STATE_SELECTING;
    c->xid = 0x12345678; // Dummy transaction ID

    // In a real implementation:
    // 1. Build DHCP DISCOVER packet
    // 2. Wrap in UDP (src: 68, dst: 67)
    // 3. Send out interface

    return 0;
}

void dhcp_client_stop(dhcp_client_t *c) {
    if (!c) return;
    c->state = DHCP_STATE_INIT;
    printf("[dhcp_client] Stopped DHCP on %s\n", c->ni->name);
}

void dhcp_client_tick(dhcp_client_t *c) {
    if (!c || c->state == DHCP_STATE_INIT) return;

    c->ticks++;
    // Basic state machine tick to handle retransmits, renewals, etc.
}
