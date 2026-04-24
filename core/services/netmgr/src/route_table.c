#include "route_table.h"
#include <bharat/runtime/freestanding_string.h>

#include <stddef.h>

static netmgr_route_t g_routes[NETMGR_MAX_ROUTES];

void netmgr_route_table_init(void) {
    for (int i = 0; i < NETMGR_MAX_ROUTES; i++) {
        g_routes[i].valid = false;
        g_routes[i].if_id = NET_IF_ID_INVALID;
        g_routes[i].af = NET_AF_UNSPEC;
        g_routes[i].metric = 0;
        memset(g_routes[i].dest, 0, 16);
        memset(g_routes[i].mask, 0, 16);
        memset(g_routes[i].gateway, 0, 16);
    }
}

static bool netmgr_route_equals(const uint8_t *a, const uint8_t *b, net_af_t af) {
    int len = (af == NET_AF_INET) ? 4 : 16;
    for(int i = 0; i < len; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

netmgr_status_t netmgr_route_add(net_if_id_t if_id, net_af_t af, const uint8_t *dest, const uint8_t *mask, const uint8_t *gateway, uint32_t metric) {
    if (if_id == NET_IF_ID_INVALID || af == NET_AF_UNSPEC || !dest || !mask) return NETMGR_STATUS_ERR_INVAL;

    netmgr_route_t *existing = NULL;
    for (int i = 0; i < NETMGR_MAX_ROUTES; i++) {
        if (g_routes[i].valid && g_routes[i].af == af &&
            netmgr_route_equals(g_routes[i].dest, dest, af) &&
            netmgr_route_equals(g_routes[i].mask, mask, af)) {
            existing = &g_routes[i];
            break;
        }
    }

    if (existing) {
        existing->if_id = if_id;
        existing->metric = metric;
        if (gateway) {
            int len = (af == NET_AF_INET) ? 4 : 16;
            memcpy(existing->gateway, gateway, len);
        } else {
            memset(existing->gateway, 0, 16);
        }
        return NETMGR_STATUS_OK;
    }

    int free_slot = -1;
    for (int i = 0; i < NETMGR_MAX_ROUTES; i++) {
        if (!g_routes[i].valid) {
            free_slot = i;
            break;
        }
    }

    if (free_slot == -1) return NETMGR_STATUS_ERR_NOSPACE;

    netmgr_route_t *entry = &g_routes[free_slot];
    entry->valid = true;
    entry->if_id = if_id;
    entry->af = af;
    entry->metric = metric;

    int len = (af == NET_AF_INET) ? 4 : 16;
    memcpy(entry->dest, dest, len);
    memcpy(entry->mask, mask, len);
    if (gateway) {
        memcpy(entry->gateway, gateway, len);
    } else {
        memset(entry->gateway, 0, len);
    }

    return NETMGR_STATUS_OK;
}

netmgr_status_t netmgr_route_remove(net_af_t af, const uint8_t *dest, const uint8_t *mask) {
    if (af == NET_AF_UNSPEC || !dest || !mask) return NETMGR_STATUS_ERR_INVAL;

    for (int i = 0; i < NETMGR_MAX_ROUTES; i++) {
        if (g_routes[i].valid && g_routes[i].af == af &&
            netmgr_route_equals(g_routes[i].dest, dest, af) &&
            netmgr_route_equals(g_routes[i].mask, mask, af)) {
            g_routes[i].valid = false;
            g_routes[i].if_id = NET_IF_ID_INVALID;
            return NETMGR_STATUS_OK;
        }
    }

    return NETMGR_STATUS_ERR_NOTFOUND;
}

netmgr_route_t* netmgr_route_lookup(net_af_t af, const uint8_t *dest) {
    if (af == NET_AF_UNSPEC || !dest) return NULL;

    netmgr_route_t *best_match = NULL;
    uint8_t best_prefix_len = 0;

    for (int i = 0; i < NETMGR_MAX_ROUTES; i++) {
        if (!g_routes[i].valid || g_routes[i].af != af) continue;

        bool match = true;
        uint8_t current_prefix_len = 0;
        int len = (af == NET_AF_INET) ? 4 : 16;
        for (int j = 0; j < len; j++) {
            if ((dest[j] & g_routes[i].mask[j]) != (g_routes[i].dest[j] & g_routes[i].mask[j])) {
                match = false;
                break;
            }
            for (int bit = 7; bit >= 0; bit--) {
                if ((g_routes[i].mask[j] >> bit) & 1) {
                    current_prefix_len++;
                } else {
                    break;
                }
            }
        }

        if (match) {
            if (!best_match || current_prefix_len > best_prefix_len ||
                (current_prefix_len == best_prefix_len && g_routes[i].metric < best_match->metric)) {
                best_match = &g_routes[i];
                best_prefix_len = current_prefix_len;
            }
        }
    }

    return best_match;
}
