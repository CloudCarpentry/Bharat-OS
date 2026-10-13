#include <bharat/coremgr/coremgr.h>
#include <string.h>

static coremgr_core_info_t g_core_registry[COREMGR_MAX_CORES];
static uint32_t g_registered_cores = 0;
static uint32_t g_topology_version = 0;

extern uint32_t g_invalid_events;
extern uint32_t g_registry_overflows;

void coremgr_registry_init(void) {
    for (uint32_t i = 0; i < COREMGR_MAX_CORES; i++) {
        g_core_registry[i].core_id = (uint32_t)-1;
        g_core_registry[i].arch_id = 0;
        g_core_registry[i].is_online = false;
        g_core_registry[i].lifecycle_state = CORE_STATE_UNKNOWN;
        g_core_registry[i].last_health_ts = 0;
        g_core_registry[i].transport_state = CORE_TRANSPORT_HEALTHY;
        g_core_registry[i].last_failure_reason = 0;
        g_core_registry[i].topology_version = 0;
    }
    g_registered_cores = 0;
    g_topology_version = 0;
    g_invalid_events = 0;
    g_registry_overflows = 0;
}

int coremgr_register_core(uint32_t core_id, uint32_t arch_id) {
    if (core_id == (uint32_t)-1) {
        coremgr_inc_invalid_event();
        return -1;
    }

    // Check for duplicate
    for (uint32_t i = 0; i < COREMGR_MAX_CORES; i++) {
        if (g_core_registry[i].core_id == core_id) {
            return -2; // Already registered
        }
    }

    // Find empty slot
    for (uint32_t i = 0; i < COREMGR_MAX_CORES; i++) {
        if (g_core_registry[i].core_id == (uint32_t)-1) {
            g_core_registry[i].core_id = core_id;
            g_core_registry[i].arch_id = arch_id;
            g_core_registry[i].is_online = false;
            g_core_registry[i].lifecycle_state = CORE_STATE_DISCOVERED;
            g_core_registry[i].last_health_ts = 0; // Or current time
            g_core_registry[i].transport_state = CORE_TRANSPORT_HEALTHY;
            g_core_registry[i].last_failure_reason = 0;
            g_core_registry[i].topology_version = ++g_topology_version;
            g_registered_cores++;
            return 0;
        }
    }

    coremgr_inc_registry_overflow();
    return -3; // Overflow
}

int coremgr_get_core_info(uint32_t core_id, coremgr_core_info_t *out_info) {
    if (!out_info) {
        return -1;
    }

    for (uint32_t i = 0; i < COREMGR_MAX_CORES; i++) {
        if (g_core_registry[i].core_id == core_id) {
            *out_info = g_core_registry[i];
            return 0;
        }
    }

    return -4; // Not found
}

// Internal function to get pointer for lifecycle.c
coremgr_core_info_t* coremgr_get_core_info_ptr(uint32_t core_id) {
    for (uint32_t i = 0; i < COREMGR_MAX_CORES; i++) {
        if (g_core_registry[i].core_id == core_id) {
            return &g_core_registry[i];
        }
    }
    return NULL;
}

coremgr_core_info_t* coremgr_get_core_info_by_index(uint32_t index) {
    if (index >= COREMGR_MAX_CORES) {
        return NULL;
    }
    return &g_core_registry[index];
}
