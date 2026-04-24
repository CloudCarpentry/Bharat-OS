#include "driver_health.h"
#include <stddef.h>

#define NETMGR_MAX_INTERFACES 16

// Max drivers tracked equals max interfaces for now
static netmgr_driver_health_t g_driver_health[NETMGR_MAX_INTERFACES];

void netmgr_driver_health_init(void) {
    for (int i = 0; i < NETMGR_MAX_INTERFACES; i++) {
        g_driver_health[i].valid = false;
        g_driver_health[i].if_id = NET_IF_ID_INVALID;
        g_driver_health[i].state = DRIVER_HEALTH_UNKNOWN;
        g_driver_health[i].restart_count = 0;
        g_driver_health[i].last_heartbeat = 0;
    }
}

netmgr_status_t netmgr_driver_health_register(net_if_id_t if_id) {
    if (if_id == NET_IF_ID_INVALID) {
        return NETMGR_STATUS_ERR_INVAL;
    }

    if (netmgr_driver_health_get(if_id)) {
        return NETMGR_STATUS_OK; // Already registered
    }

    int free_slot = -1;
    for (int i = 0; i < NETMGR_MAX_INTERFACES; i++) {
        if (!g_driver_health[i].valid) {
            free_slot = i;
            break;
        }
    }

    if (free_slot == -1) {
        return NETMGR_STATUS_ERR_NOSPACE;
    }

    g_driver_health[free_slot].valid = true;
    g_driver_health[free_slot].if_id = if_id;
    g_driver_health[free_slot].state = DRIVER_HEALTH_OK;
    g_driver_health[free_slot].restart_count = 0;
    g_driver_health[free_slot].last_heartbeat = 0; // Requires a tick/time source

    return NETMGR_STATUS_OK;
}

netmgr_status_t netmgr_driver_health_unregister(net_if_id_t if_id) {
    if (if_id == NET_IF_ID_INVALID) {
        return NETMGR_STATUS_ERR_INVAL;
    }

    for (int i = 0; i < NETMGR_MAX_INTERFACES; i++) {
        if (g_driver_health[i].valid && g_driver_health[i].if_id == if_id) {
            g_driver_health[i].valid = false;
            g_driver_health[i].if_id = NET_IF_ID_INVALID;
            return NETMGR_STATUS_OK;
        }
    }

    return NETMGR_STATUS_ERR_NOTFOUND;
}

netmgr_status_t netmgr_driver_health_report(net_if_id_t if_id, netmgr_driver_health_state_t state) {
    netmgr_driver_health_t *health = netmgr_driver_health_get(if_id);
    if (!health) {
        return NETMGR_STATUS_ERR_NOTFOUND;
    }

    health->state = state;
    // update heartbeat time if OK
    if (state == DRIVER_HEALTH_OK) {
        health->last_heartbeat = 1; // tick placeholder
    }

    return NETMGR_STATUS_OK;
}

netmgr_status_t netmgr_driver_health_request_restart(net_if_id_t if_id) {
    netmgr_driver_health_t *health = netmgr_driver_health_get(if_id);
    if (!health) {
        return NETMGR_STATUS_ERR_NOTFOUND;
    }

    // State machine stub: Transition to RESTARTING and inc counter.
    // Real system would signal the process manager or driver service here.
    health->state = DRIVER_HEALTH_RESTARTING;
    health->restart_count++;

    // In phase 3, we just record the restart intention.
    return NETMGR_STATUS_OK;
}

netmgr_driver_health_t* netmgr_driver_health_get(net_if_id_t if_id) {
    if (if_id == NET_IF_ID_INVALID) {
        return NULL;
    }

    for (int i = 0; i < NETMGR_MAX_INTERFACES; i++) {
        if (g_driver_health[i].valid && g_driver_health[i].if_id == if_id) {
            return &g_driver_health[i];
        }
    }

    return NULL;
}
