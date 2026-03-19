#ifndef NETMGR_ADDRESS_TABLE_H
#define NETMGR_ADDRESS_TABLE_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/net/net.h>
#include <bharat/network/netmgr_ipc.h>

#define NETMGR_MAX_ADDRESSES 32

typedef struct {
    net_if_id_t if_id;
    net_af_t af;
    uint8_t addr[16];
    uint8_t prefix_len;
    bool valid;
} netmgr_addr_t;

void netmgr_addr_table_init(void);

netmgr_status_t netmgr_addr_add(net_if_id_t if_id, net_af_t af, const uint8_t *addr, uint8_t prefix_len);

netmgr_status_t netmgr_addr_remove(net_if_id_t if_id, net_af_t af, const uint8_t *addr, uint8_t prefix_len);

netmgr_addr_t* netmgr_addr_get(net_if_id_t if_id, net_af_t af, const uint8_t *addr, uint8_t prefix_len);

#endif // NETMGR_ADDRESS_TABLE_H
