#include "neighbor_cache.h"
#include <string.h>
#include <stddef.h>

static netmgr_neighbor_t g_neighbors[NETMGR_MAX_NEIGHBORS];

void netmgr_neighbor_cache_init(void) {
    for (int i = 0; i < NETMGR_MAX_NEIGHBORS; i++) {
        g_neighbors[i].valid = false;
        g_neighbors[i].if_id = NET_IF_ID_INVALID;
        g_neighbors[i].af = NET_AF_UNSPEC;
        g_neighbors[i].state = NEIGHBOR_STATE_FAILED;
        g_neighbors[i].expires = 0;
        memset(g_neighbors[i].ip_addr, 0, 16);
        memset(g_neighbors[i].mac_addr, 0, NETMGR_MAX_MAC_LEN);
    }
}

static bool netmgr_neighbor_equals(const uint8_t *a, const uint8_t *b, net_af_t af) {
    int len = (af == NET_AF_INET) ? 4 : 16;
    for(int i = 0; i < len; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

netmgr_status_t netmgr_neighbor_add(net_if_id_t if_id, net_af_t af, const uint8_t *ip_addr, const uint8_t *mac_addr, neighbor_state_t state) {
    if (if_id == NET_IF_ID_INVALID || af == NET_AF_UNSPEC || !ip_addr || !mac_addr) return NETMGR_STATUS_ERR_INVAL;

    netmgr_neighbor_t *existing = NULL;
    for (int i = 0; i < NETMGR_MAX_NEIGHBORS; i++) {
        if (g_neighbors[i].valid && g_neighbors[i].if_id == if_id &&
            g_neighbors[i].af == af && netmgr_neighbor_equals(g_neighbors[i].ip_addr, ip_addr, af)) {
            existing = &g_neighbors[i];
            break;
        }
    }

    if (existing) {
        memcpy(existing->mac_addr, mac_addr, NETMGR_MAX_MAC_LEN);
        existing->state = state;
        existing->expires = 300;
        return NETMGR_STATUS_OK;
    }

    int free_slot = -1;
    for (int i = 0; i < NETMGR_MAX_NEIGHBORS; i++) {
        if (!g_neighbors[i].valid) {
            free_slot = i;
            break;
        }
    }

    if (free_slot == -1) return NETMGR_STATUS_ERR_NOSPACE;

    netmgr_neighbor_t *entry = &g_neighbors[free_slot];
    entry->valid = true;
    entry->if_id = if_id;
    entry->af = af;
    entry->state = state;
    entry->expires = 300;

    int len = (af == NET_AF_INET) ? 4 : 16;
    memcpy(entry->ip_addr, ip_addr, len);
    memcpy(entry->mac_addr, mac_addr, NETMGR_MAX_MAC_LEN);

    return NETMGR_STATUS_OK;
}

netmgr_status_t netmgr_neighbor_remove(net_if_id_t if_id, net_af_t af, const uint8_t *ip_addr) {
    if (if_id == NET_IF_ID_INVALID || af == NET_AF_UNSPEC || !ip_addr) return NETMGR_STATUS_ERR_INVAL;

    for (int i = 0; i < NETMGR_MAX_NEIGHBORS; i++) {
        if (g_neighbors[i].valid && g_neighbors[i].if_id == if_id &&
            g_neighbors[i].af == af && netmgr_neighbor_equals(g_neighbors[i].ip_addr, ip_addr, af)) {
            g_neighbors[i].valid = false;
            g_neighbors[i].if_id = NET_IF_ID_INVALID;
            return NETMGR_STATUS_OK;
        }
    }

    return NETMGR_STATUS_ERR_NOTFOUND;
}

netmgr_status_t netmgr_neighbor_flush(net_if_id_t if_id) {
    if (if_id == NET_IF_ID_INVALID) return NETMGR_STATUS_ERR_INVAL;

    for (int i = 0; i < NETMGR_MAX_NEIGHBORS; i++) {
        if (g_neighbors[i].valid && g_neighbors[i].if_id == if_id) {
            g_neighbors[i].valid = false;
            g_neighbors[i].if_id = NET_IF_ID_INVALID;
        }
    }

    return NETMGR_STATUS_OK;
}

netmgr_neighbor_t* netmgr_neighbor_lookup(net_if_id_t if_id, net_af_t af, const uint8_t *ip_addr) {
    if (if_id == NET_IF_ID_INVALID || af == NET_AF_UNSPEC || !ip_addr) return NULL;

    for (int i = 0; i < NETMGR_MAX_NEIGHBORS; i++) {
        if (g_neighbors[i].valid && g_neighbors[i].if_id == if_id &&
            g_neighbors[i].af == af && netmgr_neighbor_equals(g_neighbors[i].ip_addr, ip_addr, af)) {
            return &g_neighbors[i];
        }
    }

    return NULL;
}
