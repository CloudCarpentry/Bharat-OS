#include "hal/hal_mm.h"

void hal_mm_get_zone_limits(hal_mm_zone_limits_t *out) {
    if (!out) return;
    out->dma_low_start = 0;
    out->dma_low_end = 0;
    out->dma32_start = 0;
#if __riscv_xlen == 32
    out->dma32_end = 0xFFFFFFFF;
    out->normal_start = 0;
    out->normal_end = 0xFFFFFFFF;
#else
    out->dma32_end = 0xFFFFFFFF;
    out->normal_start = 0x100000000ULL;
    out->normal_end = (uintptr_t)-1;
#endif
    out->flags = 0;
}

void hal_mm_backend_caps(hal_mm_backend_caps_t *out) {
    if (!out) return;
#if __riscv_xlen == 32
    out->kind = HAL_MM_BACKEND_MMU_LITE;
#else
    out->kind = HAL_MM_BACKEND_MMU_FULL;
#endif
    out->map_granule = 4096;
    out->protect_granule = 4096;
    out->alloc_granule = 4096;
    out->max_regions = 0;
    out->flags = 1;
}
