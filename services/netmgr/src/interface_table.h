#ifndef NETMGR_INTERFACE_TABLE_H
#define NETMGR_INTERFACE_TABLE_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/net/net.h>
#include <bharat/network/netmgr_ipc.h>

#define NETMGR_MAX_INTERFACES 16

typedef struct {
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint32_t rx_packets;
    uint32_t tx_packets;
    uint32_t rx_errors;
    uint32_t tx_errors;
} netmgr_iface_stats_t;

typedef struct {
    net_if_id_t if_id;
    char name[NETMGR_MAX_IF_NAME_LEN];
    uint8_t mac[NETMGR_MAX_MAC_LEN];
    uint32_t mtu;
    bool admin_up;
    net_link_state_t link_state;
    netmgr_iface_stats_t stats;
} netmgr_iface_t;

void netmgr_iface_table_init(void);

netmgr_status_t netmgr_iface_create(const char *name, const uint8_t *mac, uint32_t mtu, net_if_id_t *out_if_id);

netmgr_status_t netmgr_iface_delete(net_if_id_t if_id);

netmgr_status_t netmgr_iface_set_admin_state(net_if_id_t if_id, bool admin_up);

netmgr_iface_t* netmgr_iface_get(net_if_id_t if_id);

#endif // NETMGR_INTERFACE_TABLE_H
