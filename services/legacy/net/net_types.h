#ifndef SERVICES_NET_TYPES_H
#define SERVICES_NET_TYPES_H

#include <stdint.h>
#include <net/netdev.h>
#include <net/net_core.h>

#define MAX_IF_NAME_LEN 16

/*
 * Interface metadata shared between control and data plane
 */
typedef struct net_if {
    uint32_t if_id;
    char name[MAX_IF_NAME_LEN];
    uint8_t mac[MAC_ADDR_LEN];
    uint32_t mtu;
    net_link_state_t link_state;

    /* Device stats */
    netdev_stats_t stats;

    /* Private device handle, could be capability or pointers in real hw */
    void* priv;
} net_if_t;

#endif /* SERVICES_NET_TYPES_H */
