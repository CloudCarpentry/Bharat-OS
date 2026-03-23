#include "bharat/skb.h"
#include <hal/hal.h>

#ifndef BHARAT_MAX_CPUS
#define BHARAT_MAX_CPUS 32
#endif

#define SKB_THERMAL_PRESSURE_HIGH 80U

static core_topology_t g_core_topology[BHARAT_MAX_CPUS];
static bool g_skb_initialized = false;
static bool g_global_thermal_emergency = false;

int skb_init_topology(void) {
    for (uint32_t i = 0; i < BHARAT_MAX_CPUS; i++) {
        g_core_topology[i].core_id = i;
        g_core_topology[i].numa_node = 0;
        g_core_topology[i].l3_cache_id = 0;
        g_core_topology[i].thermal_pressure_pct = 0U;
    }
    g_global_thermal_emergency = false;
    g_skb_initialized = true;
    return 0;
}

void skb_register_core(uint32_t core_id, uint32_t numa_node, uint32_t l3_cache_id) {
    if (core_id < BHARAT_MAX_CPUS) {
        g_core_topology[core_id].numa_node = numa_node;
        g_core_topology[core_id].l3_cache_id = l3_cache_id;
    }
}

void skb_set_core_thermal_pressure(uint32_t core_id, uint8_t pressure_pct) {
    if (core_id >= BHARAT_MAX_CPUS) {
        return;
    }

    if (pressure_pct > 100U) {
        pressure_pct = 100U;
    }
    g_core_topology[core_id].thermal_pressure_pct = pressure_pct;
}

void skb_set_global_thermal_emergency(bool emergency_active) {
    g_global_thermal_emergency = emergency_active;
}

skb_transport_t skb_pick_transport(uint32_t source_core, uint32_t target_core) {
    if (!g_skb_initialized || source_core >= BHARAT_MAX_CPUS || target_core >= BHARAT_MAX_CPUS) {
        return SKB_TRANSPORT_UNKNOWN;
    }

    if (g_global_thermal_emergency ||
        g_core_topology[source_core].thermal_pressure_pct >= SKB_THERMAL_PRESSURE_HIGH ||
        g_core_topology[target_core].thermal_pressure_pct >= SKB_THERMAL_PRESSURE_HIGH) {
        return SKB_TRANSPORT_IPI_ASSISTED;
    }

    // If on the same L3 cache group, pure shared memory URPC is optimal.
    if (g_core_topology[source_core].l3_cache_id == g_core_topology[target_core].l3_cache_id) {
        return SKB_TRANSPORT_URPC_SHARED_MEM;
    }

    // Otherwise cross-NUMA or cross-L3 might benefit from IPI-assisted notification.
    return SKB_TRANSPORT_IPI_ASSISTED;
}
