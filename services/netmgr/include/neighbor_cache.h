#ifndef NETMGR_NEIGHBOR_CACHE_H
#define NETMGR_NEIGHBOR_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/net/net.h>
#include <bharat/network/netmgr_ipc.h>

#define NETMGR_MAX_NEIGHBORS 128

typedef enum {
    NEIGHBOR_STATE_INCOMPLETE,
    NEIGHBOR_STATE_REACHABLE,
    NEIGHBOR_STATE_STALE,
    NEIGHBOR_STATE_DELAY,
    NEIGHBOR_STATE_PROBE,
    NEIGHBOR_STATE_FAILED
} neighbor_state_t;

typedef struct {
    net_if_id_t if_id;
    net_af_t af;
    uint8_t ip_addr[16];
    uint8_t mac_addr[NETMGR_MAX_MAC_LEN];
    neighbor_state_t state;
    uint32_t expires;
    bool valid;
} netmgr_neighbor_t;

void netmgr_neighbor_cache_init(void);
netmgr_status_t netmgr_neighbor_add(net_if_id_t if_id, net_af_t af, const uint8_t *ip_addr, const uint8_t *mac_addr, neighbor_state_t state);
netmgr_status_t netmgr_neighbor_remove(net_if_id_t if_id, net_af_t af, const uint8_t *ip_addr);
netmgr_status_t netmgr_neighbor_flush(net_if_id_t if_id);
netmgr_neighbor_t* netmgr_neighbor_lookup(net_if_id_t if_id, net_af_t af, const uint8_t *ip_addr);

#endif // NETMGR_NEIGHBOR_CACHE_H
