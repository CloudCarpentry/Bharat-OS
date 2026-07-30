#include "route_table.h"
#include <bharat/runtime/freestanding_string.h>

#include <stddef.h>

static netmgr_route_t g_routes[NETMGR_MAX_ROUTES];

static bool route_af_len(net_af_t af, size_t *len) {
    switch (af) {
        case NET_AF_INET:
            *len = 4;
            return true;
        case NET_AF_INET6:
            *len = 16;
            return true;
        default:
            return false;
    }
}

static bool route_mask_validate(const uint8_t *mask, size_t len, uint8_t *out_prefix_len) {
    bool zero_seen = false;
    uint32_t ones_count = 0;

    for (size_t i = 0; i < len; i++) {
        uint8_t byte = mask[i];
        for (int bit = 7; bit >= 0; bit--) {
            bool is_one = (byte >> bit) & 1;
            if (is_one) {
                if (zero_seen) {
                    return false;
                }
                ones_count++;
            } else {
                zero_seen = true;
            }
        }
    }

    if (out_prefix_len) {
        *out_prefix_len = (uint8_t)ones_count;
    }
    return true;
}

static void route_entry_reset(netmgr_route_t *entry) {
    entry->valid = false;
    entry->if_id = NET_IF_ID_INVALID;
    entry->af = NET_AF_UNSPEC;
    entry->metric = 0;
    entry->prefix_len = 0;
    memset(entry->dest, 0, sizeof(entry->dest));
    memset(entry->mask, 0, sizeof(entry->mask));
    memset(entry->gateway, 0, sizeof(entry->gateway));
}

void netmgr_route_table_init(void) {
    for (int i = 0; i < NETMGR_MAX_ROUTES; i++) {
        route_entry_reset(&g_routes[i]);
    }
}

static bool netmgr_route_equals(const uint8_t *a, const uint8_t *b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

netmgr_status_t netmgr_route_add(net_if_id_t if_id, net_af_t af, const uint8_t *dest, const uint8_t *mask, const uint8_t *gateway, uint32_t metric) {
    if (if_id == NET_IF_ID_INVALID || !dest || !mask) {
        return NETMGR_STATUS_ERR_INVAL;
    }

    size_t len = 0;
    if (!route_af_len(af, &len)) {
        return NETMGR_STATUS_ERR_INVAL;
    }

    uint8_t prefix_len = 0;
    if (!route_mask_validate(mask, len, &prefix_len)) {
        return NETMGR_STATUS_ERR_INVAL;
    }

    // Canonicalize destination: dest & mask
    uint8_t canonical_dest[16];
    memset(canonical_dest, 0, sizeof(canonical_dest));
    for (size_t i = 0; i < len; i++) {
        canonical_dest[i] = dest[i] & mask[i];
    }

    netmgr_route_t *existing = NULL;
    for (int i = 0; i < NETMGR_MAX_ROUTES; i++) {
        if (g_routes[i].valid && g_routes[i].af == af &&
            netmgr_route_equals(g_routes[i].dest, canonical_dest, len) &&
            netmgr_route_equals(g_routes[i].mask, mask, len)) {
            existing = &g_routes[i];
            break;
        }
    }

    if (existing) {
        existing->if_id = if_id;
        existing->metric = metric;
        existing->prefix_len = prefix_len;
        memset(existing->gateway, 0, sizeof(existing->gateway));
        if (gateway) {
            memcpy(existing->gateway, gateway, len);
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

    if (free_slot == -1) {
        return NETMGR_STATUS_ERR_NOSPACE;
    }

    netmgr_route_t *entry = &g_routes[free_slot];
    route_entry_reset(entry);

    entry->valid = true;
    entry->if_id = if_id;
    entry->af = af;
    entry->metric = metric;
    entry->prefix_len = prefix_len;

    memcpy(entry->dest, canonical_dest, len);
    memcpy(entry->mask, mask, len);
    if (gateway) {
        memcpy(entry->gateway, gateway, len);
    }

    return NETMGR_STATUS_OK;
}

netmgr_status_t netmgr_route_remove(net_af_t af, const uint8_t *dest, const uint8_t *mask) {
    if (!dest || !mask) {
        return NETMGR_STATUS_ERR_INVAL;
    }

    size_t len = 0;
    if (!route_af_len(af, &len)) {
        return NETMGR_STATUS_ERR_INVAL;
    }

    if (!route_mask_validate(mask, len, NULL)) {
        return NETMGR_STATUS_ERR_INVAL;
    }

    // Canonicalize supplied destination
    uint8_t canonical_dest[16];
    memset(canonical_dest, 0, sizeof(canonical_dest));
    for (size_t i = 0; i < len; i++) {
        canonical_dest[i] = dest[i] & mask[i];
    }

    for (int i = 0; i < NETMGR_MAX_ROUTES; i++) {
        if (g_routes[i].valid && g_routes[i].af == af &&
            netmgr_route_equals(g_routes[i].dest, canonical_dest, len) &&
            netmgr_route_equals(g_routes[i].mask, mask, len)) {

            route_entry_reset(&g_routes[i]);
            return NETMGR_STATUS_OK;
        }
    }

    return NETMGR_STATUS_ERR_NOTFOUND;
}

netmgr_route_t* netmgr_route_lookup(net_af_t af, const uint8_t *dest) {
    if (!dest) {
        return NULL;
    }

    size_t len = 0;
    if (!route_af_len(af, &len)) {
        return NULL;
    }

    netmgr_route_t *best_match = NULL;

    for (int i = 0; i < NETMGR_MAX_ROUTES; i++) {
        if (!g_routes[i].valid || g_routes[i].af != af) {
            continue;
        }

        bool match = true;
        for (size_t j = 0; j < len; j++) {
            if ((dest[j] & g_routes[i].mask[j]) != g_routes[i].dest[j]) {
                match = false;
                break;
            }
        }

        if (match) {
            if (!best_match) {
                best_match = &g_routes[i];
            } else if (g_routes[i].prefix_len > best_match->prefix_len) {
                best_match = &g_routes[i];
            } else if (g_routes[i].prefix_len == best_match->prefix_len) {
                if (g_routes[i].metric < best_match->metric) {
                    best_match = &g_routes[i];
                }
            }
        }
    }

    return best_match;
}
