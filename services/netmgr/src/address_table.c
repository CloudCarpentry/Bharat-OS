#include "address_table.h"
#include <bharat/runtime/freestanding_string.h>

#include <stddef.h>

static netmgr_addr_t g_addresses[NETMGR_MAX_ADDRESSES];

void netmgr_addr_table_init(void) {
    for (int i = 0; i < NETMGR_MAX_ADDRESSES; i++) {
        g_addresses[i].valid = false;
        g_addresses[i].if_id = NET_IF_ID_INVALID;
        g_addresses[i].af = NET_AF_UNSPEC;
        g_addresses[i].prefix_len = 0;
        memset(g_addresses[i].addr, 0, 16);
    }
}

static bool netmgr_addr_equals(const uint8_t *a, const uint8_t *b, net_af_t af) {
    int len = (af == NET_AF_INET) ? 4 : 16;
    for(int i = 0; i < len; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

netmgr_status_t netmgr_addr_add(net_if_id_t if_id, net_af_t af, const uint8_t *addr, uint8_t prefix_len) {
    if (if_id == NET_IF_ID_INVALID || af == NET_AF_UNSPEC || !addr) return NETMGR_STATUS_ERR_INVAL;

    if (netmgr_addr_get(if_id, af, addr, prefix_len) != NULL) return NETMGR_STATUS_OK;

    int free_slot = -1;
    for (int i = 0; i < NETMGR_MAX_ADDRESSES; i++) {
        if (!g_addresses[i].valid) {
            free_slot = i;
            break;
        }
    }

    if (free_slot == -1) return NETMGR_STATUS_ERR_NOSPACE;

    netmgr_addr_t *entry = &g_addresses[free_slot];
    entry->valid = true;
    entry->if_id = if_id;
    entry->af = af;
    entry->prefix_len = prefix_len;

    int len = (af == NET_AF_INET) ? 4 : 16;
    memcpy(entry->addr, addr, len);

    return NETMGR_STATUS_OK;
}

netmgr_status_t netmgr_addr_remove(net_if_id_t if_id, net_af_t af, const uint8_t *addr, uint8_t prefix_len) {
    netmgr_addr_t *entry = netmgr_addr_get(if_id, af, addr, prefix_len);
    if (!entry) return NETMGR_STATUS_ERR_NOTFOUND;
    entry->valid = false;
    entry->if_id = NET_IF_ID_INVALID;
    return NETMGR_STATUS_OK;
}

netmgr_addr_t* netmgr_addr_get(net_if_id_t if_id, net_af_t af, const uint8_t *addr, uint8_t prefix_len) {
    if (if_id == NET_IF_ID_INVALID || af == NET_AF_UNSPEC || !addr) return NULL;

    for (int i = 0; i < NETMGR_MAX_ADDRESSES; i++) {
        if (g_addresses[i].valid && g_addresses[i].if_id == if_id &&
            g_addresses[i].af == af && g_addresses[i].prefix_len == prefix_len) {
            if (netmgr_addr_equals(g_addresses[i].addr, addr, af)) {
                return &g_addresses[i];
            }
        }
    }

    return NULL;
}
