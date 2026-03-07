#include "../../include/numa.h"

#include <stddef.h>

static numa_node_descriptor_t g_nodes[NUMA_MAX_NODES];
static uint32_t g_active_nodes;

int numa_discover_topology(void) {
    // Baseline: single NUMA node until ACPI SRAT/FDT parsing lands.
    g_nodes[0].node_id = 0U;
    g_nodes[0].start_addr = 0U;
    g_nodes[0].size_bytes = 0U;
    g_nodes[0].cpu_count = 1U;
    g_nodes[0].active = 1U;

    for (uint32_t i = 1U; i < NUMA_MAX_NODES; ++i) {
        g_nodes[i].active = 0U;
    }

    g_active_nodes = 1U;
    return 0;
}

memory_node_id_t numa_get_current_node(void) {
    return NUMA_NODE_LOCAL;
}

int numa_set_node_descriptor(memory_node_id_t node_id,
                             uint64_t start_addr,
                             uint64_t size_bytes,
                             uint32_t cpu_count) {
    if (node_id >= NUMA_MAX_NODES || cpu_count == 0U) {
        return -1;
    }

    g_nodes[node_id].node_id = node_id;
    g_nodes[node_id].start_addr = start_addr;
    g_nodes[node_id].size_bytes = size_bytes;
    g_nodes[node_id].cpu_count = cpu_count;
    g_nodes[node_id].active = 1U;

    if ((uint32_t)(node_id + 1U) > g_active_nodes) {
        g_active_nodes = (uint32_t)(node_id + 1U);
    }

    return 0;
}

int numa_get_node_descriptor(memory_node_id_t node_id, numa_node_descriptor_t* out_desc) {
    if (!out_desc || node_id >= NUMA_MAX_NODES || g_nodes[node_id].active == 0U) {
        return -1;
    }

    *out_desc = g_nodes[node_id];
    return 0;
}

uint32_t numa_active_node_count(void) {
    return g_active_nodes;
}
