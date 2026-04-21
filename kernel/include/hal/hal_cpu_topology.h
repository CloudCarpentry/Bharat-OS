#ifndef BHARAT_HAL_CPU_TOPOLOGY_H
#define BHARAT_HAL_CPU_TOPOLOGY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t discovered_cpu_count;
    bool smp_available;
    bool homogeneous_cores;
} hal_cpu_topology_info_t;

bool hal_cpu_topology_query(hal_cpu_topology_info_t *out);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_HAL_CPU_TOPOLOGY_H
