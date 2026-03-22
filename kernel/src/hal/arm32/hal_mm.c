#include "hal/hal_mm.h"

void hal_mm_get_zone_limits(hal_mm_zone_limits_t *out) {
    if (!out) return;
    // For 32-bit architectures, DMA32 limit covers all memory.
    // Normal zone is mostly unified with DMA32.
    out->dma_low_start = 0;
    out->dma_low_end = 0;
    out->dma32_start = 0;
    out->dma32_end = 0xFFFFFFFF; // All 32-bit memory
    out->normal_start = 0;
    out->normal_end = 0xFFFFFFFF;
    out->flags = 0;
}

void hal_mm_backend_caps(hal_mm_backend_caps_t *out) {
    if (!out) return;
    out->kind = HAL_MM_BACKEND_MPU_ONLY; // Typical for arm32 RTOS profile
    out->map_granule = 32; // MPU region minimum size
    out->protect_granule = 32;
    out->alloc_granule = 4096;
    out->max_regions = 16;
    out->flags = 0; // No sparse map
}
