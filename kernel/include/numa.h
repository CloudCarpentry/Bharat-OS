#ifndef BHARAT_OS_NUMA_H
#define BHARAT_OS_NUMA_H

#include <stdint.h>
#include <stddef.h>

/**
 * NUMA-Ready Interface Definitions (ADR-006)
 * Bharat-OS v1 implements single-node allocators but passes these
 * topology hints via the API to ensure forward capability for Bharat-Cloud
 * deployments on massively scaled multi-socket servers.
 */

// A distinct memory or compute node in the topology.
typedef uint16_t memory_node_id_t;

#define NUMA_NODE_LOCAL 0
#define NUMA_NODE_ANY   0xFFFF

/**
 * NUMA distance matrix stub.
 * Represents the relative latency between two nodes.
 * Local node distance is typically 10.
 */
typedef struct {
    memory_node_id_t node_a;
    memory_node_id_t node_b;
    uint32_t distance_cost;
} numa_distance_t;

/**
 * Affinity definition for scheduler hints.
 */
typedef struct {
    memory_node_id_t preferred_node;
    uint8_t strict_bind; // If 1, fail if memory isn't available on preferred node
} numa_affinity_t;

// Forward declaration of topology discovery (to be filled by ACPI/SRAT later)
int numa_discover_topology();

// Gets the ID of the node currently executing the thread
memory_node_id_t numa_get_current_node();

#endif // BHARAT_OS_NUMA_H
