#pragma once

#include "net/netif.h"
#include <stdint.h>

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

typedef enum {
    DHCP_STATE_INIT = 0,
    DHCP_STATE_SELECTING,
    DHCP_STATE_REQUESTING,
    DHCP_STATE_BOUND,
    DHCP_STATE_RENEWING,
    DHCP_STATE_REBINDING
} dhcp_state_t;

typedef struct {
    netif_t *ni;
    dhcp_state_t state;
    uint32_t xid;
    ipv4_addr_t offered_ip;
    ipv4_addr_t server_ip;
    uint32_t lease_time;
    uint32_t t1;
    uint32_t t2;
    uint32_t ticks;
} dhcp_client_t;

void dhcp_client_init(dhcp_client_t *c, netif_t *ni);
int dhcp_client_start(dhcp_client_t *c);
void dhcp_client_stop(dhcp_client_t *c);
void dhcp_client_tick(dhcp_client_t *c);
