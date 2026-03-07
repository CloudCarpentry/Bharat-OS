#ifndef BHARAT_OS_NUMA_H
#define BHARAT_OS_NUMA_H

#include <stdint.h>
#include <stddef.h>

/**
 * NUMA-Ready Interface Definitions (ADR-006)
 */

typedef uint16_t memory_node_id_t;

#define NUMA_NODE_LOCAL 0
#define NUMA_NODE_ANY   0xFFFF
#define NUMA_MAX_NODES 8

typedef struct {
    memory_node_id_t node_a;
    memory_node_id_t node_b;
    uint32_t distance_cost;
} numa_distance_t;

typedef struct {
    memory_node_id_t preferred_node;
    uint8_t strict_bind;
} numa_affinity_t;

typedef struct {
    memory_node_id_t node_id;
    uint64_t start_addr;
    uint64_t size_bytes;
    uint32_t cpu_count;
    uint8_t active;
} numa_node_descriptor_t;

int numa_discover_topology();
memory_node_id_t numa_get_current_node();

int numa_set_node_descriptor(memory_node_id_t node_id,
                             uint64_t start_addr,
                             uint64_t size_bytes,
                             uint32_t cpu_count);
int numa_get_node_descriptor(memory_node_id_t node_id, numa_node_descriptor_t* out_desc);
uint32_t numa_active_node_count(void);

#endif // BHARAT_OS_NUMA_H
