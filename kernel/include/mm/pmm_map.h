#ifndef BHARAT_MM_PMM_MAP_H
#define BHARAT_MM_PMM_MAP_H

#include <stddef.h>
#include <stdint.h>
#include "pmm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PMM_REGION_TYPE_USABLE = 0,
    PMM_REGION_TYPE_RESERVED,
    PMM_REGION_TYPE_KERNEL,
    PMM_REGION_TYPE_MODULES,
    PMM_REGION_TYPE_ACPI_RECLAIM,
    PMM_REGION_TYPE_ACPI_NVS,
    PMM_REGION_TYPE_DEVICE,
    PMM_REGION_TYPE_DEFECTIVE,
} pmm_region_type_t;

typedef struct {
    uint64_t base_addr;
    uint64_t length;
    pmm_region_type_t type;
    uint32_t attributes;
    uint32_t numa_node;
    pmm_zone_t zone_hint;
} pmm_memory_region_t;

#define MAX_PMM_REGIONS 256

typedef struct {
    uint32_t region_count;
    pmm_memory_region_t regions[MAX_PMM_REGIONS];
} pmm_memory_map_t;

int pmm_register_region(uint64_t base, uint64_t len, pmm_region_type_t type, uint32_t numa_node, uint32_t attrs);
int pmm_ingest_memory_map(const pmm_memory_map_t *map);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_PMM_MAP_H