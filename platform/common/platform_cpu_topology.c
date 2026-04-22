#include <hal/hal_cpu_topology.h>

/**
 * Generic fallback for CPU topology query.
 * This is meant to be overridden by board-specific logic where applicable.
 * Returns 1 CPU without SMP.
 */
__attribute__((weak)) bool hal_cpu_topology_query(hal_cpu_topology_info_t *out) {
    if (!out) {
        return false;
    }

    out->discovered_cpu_count = 1;
    out->valid_cpu_mask = 0x1U;
    out->performance_cluster_mask = out->valid_cpu_mask;
    out->efficiency_cluster_mask = 0U;
    out->smp_available = false;
    out->homogeneous_cores = true;

    return true;
}
