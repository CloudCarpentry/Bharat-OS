#ifndef NETMGR_ROUTE_TABLE_H
#define NETMGR_ROUTE_TABLE_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/net/net.h>
#include <bharat/network/netmgr_ipc.h>

#define NETMGR_MAX_ROUTES 64

typedef struct {
    net_if_id_t if_id;
    net_af_t af;
    uint8_t dest[16];
    uint8_t mask[16];
    uint8_t gateway[16];
    uint32_t metric;
    bool valid;
} netmgr_route_t;

void netmgr_route_table_init(void);
netmgr_status_t netmgr_route_add(net_if_id_t if_id, net_af_t af, const uint8_t *dest, const uint8_t *mask, const uint8_t *gateway, uint32_t metric);
netmgr_status_t netmgr_route_remove(net_af_t af, const uint8_t *dest, const uint8_t *mask);
netmgr_route_t* netmgr_route_lookup(net_af_t af, const uint8_t *dest);

#endif // NETMGR_ROUTE_TABLE_H
