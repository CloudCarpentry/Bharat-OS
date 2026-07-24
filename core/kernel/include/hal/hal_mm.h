#ifndef BHARAT_HAL_MM_H
#define BHARAT_HAL_MM_H

#include <stdint.h>
#include <stddef.h>

typedef struct hal_mm_zone_limits {
    uintptr_t dma_low_start;
    uintptr_t dma_low_end;
    uintptr_t dma32_start;
    uintptr_t dma32_end;
    uintptr_t normal_start;
    uintptr_t normal_end;
    uint32_t flags;
} hal_mm_zone_limits_t;

// Get boundaries for MM zones (like DMA32) for the PMM
void hal_mm_get_zone_limits(hal_mm_zone_limits_t *out);

typedef enum {
    HAL_MM_BACKEND_NONE = 0,
    HAL_MM_BACKEND_MPU_ONLY,
    HAL_MM_BACKEND_MMU_LITE,
    HAL_MM_BACKEND_MMU_FULL
} hal_mm_backend_kind_t;

typedef struct hal_mm_backend_caps {
    hal_mm_backend_kind_t kind;
    uintptr_t map_granule;
    uintptr_t protect_granule;
    uintptr_t alloc_granule;
    uint32_t max_regions;          // for MPU
    uint32_t flags;                // NX, USER_SPLIT, ALIASING, SPARSE_MAP, etc.
} hal_mm_backend_caps_t;

// Get capabilities of the VMM backend
void hal_mm_backend_caps(hal_mm_backend_caps_t *out);

typedef struct hal_memory_caps {
    bool supports_mmu_full;
    bool supports_mmu_lite;
    bool supports_mpu_only;
    bool supports_user_kernel_split;
    bool supports_page_protection;
    bool supports_execute_disable;
    bool supports_asid;
    bool supports_range_tlb_flush;
    uint32_t min_page_size;
    uint32_t max_address_bits;
} hal_memory_caps_t;

const hal_memory_caps_t *hal_memory_caps(void);

#endif // BHARAT_HAL_MM_H
