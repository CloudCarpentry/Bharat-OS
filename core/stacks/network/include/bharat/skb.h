#ifndef BHARAT_SKB_H
#define BHARAT_SKB_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SKB_TRANSPORT_URPC_SHARED_MEM = 0,
    SKB_TRANSPORT_IPI_ASSISTED = 1,
    SKB_TRANSPORT_UNKNOWN = 2
} skb_transport_t;

typedef struct {
    uint32_t core_id;
    uint32_t numa_node;
    uint32_t l3_cache_id;
    uint8_t thermal_pressure_pct;
} core_topology_t;

// Initialize the SKB topology map
int skb_init_topology(void);

// Register a core's topology info
void skb_register_core(uint32_t core_id, uint32_t numa_node, uint32_t l3_cache_id);

// Thermal inputs from scheduler/telemetry path
void skb_set_core_thermal_pressure(uint32_t core_id, uint8_t pressure_pct);
void skb_set_global_thermal_emergency(bool emergency_active);

// Pick the optimal transport between two cores based on topology and thermal pressure.
skb_transport_t skb_pick_transport(uint32_t source_core, uint32_t target_core);

#endif // BHARAT_SKB_H
