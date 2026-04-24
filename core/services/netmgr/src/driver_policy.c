#include "driver_policy.h"
#include <stddef.h>

#define NETMGR_MAX_INTERFACES 16

static netmgr_driver_profile_t g_profiles[NETMGR_MAX_DRIVER_PROFILES];
static netmgr_driver_binding_t g_bindings[NETMGR_MAX_INTERFACES]; // Match max interfaces

void netmgr_driver_policy_init(void) {
    for (int i = 0; i < NETMGR_MAX_DRIVER_PROFILES; i++) {
        g_profiles[i].valid = false;
        g_profiles[i].profile_id = 0;
    }

    for (int i = 0; i < NETMGR_MAX_INTERFACES; i++) {
        g_bindings[i].valid = false;
        g_bindings[i].if_id = NET_IF_ID_INVALID;
        g_bindings[i].profile_id = 0;
    }

    // Insert a default profile
    netmgr_driver_policy_add_profile(0, PROFILE_TYPE_DEFAULT, false, true, false, 1);
}

netmgr_status_t netmgr_driver_policy_add_profile(uint32_t profile_id, driver_profile_type_t type, bool tso, bool csum, bool promisc, uint32_t max_queues) {
    int free_slot = -1;

    // Check for existing or free slot
    for (int i = 0; i < NETMGR_MAX_DRIVER_PROFILES; i++) {
        if (g_profiles[i].valid && g_profiles[i].profile_id == profile_id) {
            // Update existing
            g_profiles[i].type = type;
            g_profiles[i].allow_tso = tso;
            g_profiles[i].allow_csum = csum;
            g_profiles[i].allow_promisc = promisc;
            g_profiles[i].max_queues = max_queues;
            return NETMGR_STATUS_OK;
        }
        if (!g_profiles[i].valid && free_slot == -1) {
            free_slot = i;
        }
    }

    if (free_slot == -1) {
        return NETMGR_STATUS_ERR_NOSPACE;
    }

    netmgr_driver_profile_t *p = &g_profiles[free_slot];
    p->valid = true;
    p->profile_id = profile_id;
    p->type = type;
    p->allow_tso = tso;
    p->allow_csum = csum;
    p->allow_promisc = promisc;
    p->max_queues = max_queues;

    return NETMGR_STATUS_OK;
}

netmgr_status_t netmgr_driver_policy_bind(net_if_id_t if_id, uint32_t profile_id) {
    if (if_id == NET_IF_ID_INVALID) {
        return NETMGR_STATUS_ERR_INVAL;
    }

    if (!netmgr_driver_policy_get_profile(profile_id)) {
        return NETMGR_STATUS_ERR_NOTFOUND;
    }

    int free_slot = -1;
    for (int i = 0; i < NETMGR_MAX_INTERFACES; i++) {
        if (g_bindings[i].valid && g_bindings[i].if_id == if_id) {
            g_bindings[i].profile_id = profile_id;
            return NETMGR_STATUS_OK;
        }
        if (!g_bindings[i].valid && free_slot == -1) {
            free_slot = i;
        }
    }

    if (free_slot == -1) {
        return NETMGR_STATUS_ERR_NOSPACE;
    }

    g_bindings[free_slot].valid = true;
    g_bindings[free_slot].if_id = if_id;
    g_bindings[free_slot].profile_id = profile_id;

    return NETMGR_STATUS_OK;
}

netmgr_driver_profile_t* netmgr_driver_policy_get_profile(uint32_t profile_id) {
    for (int i = 0; i < NETMGR_MAX_DRIVER_PROFILES; i++) {
        if (g_profiles[i].valid && g_profiles[i].profile_id == profile_id) {
            return &g_profiles[i];
        }
    }
    return NULL;
}

netmgr_driver_profile_t* netmgr_driver_policy_get_binding(net_if_id_t if_id) {
    if (if_id == NET_IF_ID_INVALID) return NULL;

    for (int i = 0; i < NETMGR_MAX_INTERFACES; i++) {
        if (g_bindings[i].valid && g_bindings[i].if_id == if_id) {
            return netmgr_driver_policy_get_profile(g_bindings[i].profile_id);
        }
    }

    // Fallback to default profile 0
    return netmgr_driver_policy_get_profile(0);
}
