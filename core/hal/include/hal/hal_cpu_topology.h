#ifndef BHARAT_HAL_CPU_TOPOLOGY_H
#define BHARAT_HAL_CPU_TOPOLOGY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t discovered_cpu_count;
    uint32_t valid_cpu_mask;
    uint32_t performance_cluster_mask;
    uint32_t efficiency_cluster_mask;
    bool smp_available;
    bool homogeneous_cores;
} hal_cpu_topology_info_t;

/**
 * @brief Query the hardware platform for CPU topology information.
 * @param out Pointer to the structure to fill.
 * @return true on success, false on failure.
 */
bool hal_cpu_topology_query(hal_cpu_topology_info_t *out);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_HAL_CPU_TOPOLOGY_H
