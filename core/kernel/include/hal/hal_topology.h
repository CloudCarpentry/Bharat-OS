#ifndef BHARAT_HAL_TOPOLOGY_H
#define BHARAT_HAL_TOPOLOGY_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t node_id;
    uint32_t package_id;
    uint32_t core_id;
    uint32_t thread_id;
} bharat_cpu_topology_t;

typedef struct {
    uint32_t node_id;
    uint64_t start_addr;
    uint64_t length;
} bharat_mem_topology_t;

// Returns basic UMA topology or parses provided SRAT/DT for real NUMA maps
int hal_topology_init(void);

// Query node for given logical CPU
uint32_t hal_topology_get_cpu_node(uint32_t cpu_id);

// Query node for given physical memory address
uint32_t hal_topology_get_mem_node(uint64_t paddr);

// Query if architecture is truly NUMA
bool hal_topology_is_numa(void);

#endif
