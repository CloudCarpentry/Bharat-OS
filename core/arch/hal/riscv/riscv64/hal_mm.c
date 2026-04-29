#include <stdbool.h>
#include "hal/hal_capabilities.h"
#include "hal/hal_mm.h"
#include "hal/hal_mmu.h"
#include <string.h>

int hal_mem_get_caps(hal_mem_caps_t *caps) {
    if (!caps) return -1;

    memset(caps, 0, sizeof(hal_mem_caps_t));
    caps->model = HAL_MEMORY_MODEL_MMU_FULL;
    caps->va_bits = 48;
    caps->pa_bits = 56;
    caps->page_sizes_mask = HAL_PAGE_SIZE_4K | HAL_PAGE_SIZE_2M | HAL_PAGE_SIZE_1G;
    caps->supports_nx = true;
    caps->supports_asid = true;
    caps->supports_global = true;
    caps->supports_user_mode = true;
    caps->supports_write_protect = true;
    caps->supports_iommu = false;
    caps->supports_hugepages = true;
    caps->supports_dirty_accessed = true;
    caps->max_mpu_regions = 0;
    return 0;
}

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

const hal_arch_capabilities_t *hal_get_arch_capabilities(void) {
    static const hal_arch_capabilities_t g_riscv64_caps = {
        .arch_name = "riscv64",
        .arch_bits = 64,
        .support_level = BH_ARCH_SUPPORT_RUNTIME_SUPPORTED,
        .memory_model = BH_MEMORY_MODEL_MMU_FULL,
        .has_smp = true,
        .has_irq_controller = true,
        .has_monotonic_timer = true,
        .has_cycle_counter = true,
        .has_dma = true,
        .has_iommu = false,
        .has_cache_ops = true,
        .has_tlb_ops = true,
        .max_supported_cores = 64
    };
    return &g_riscv64_caps;
}

static const hal_memory_caps_t riscv64_memory_caps = {
    .supports_mmu_full = true,
    .supports_mmu_lite = false,
    .supports_mpu_only = false,
    .supports_user_kernel_split = true,
    .supports_page_protection = true,
    .supports_execute_disable = true,
    .supports_asid = true,
    .supports_range_tlb_flush = true,
    .min_page_size = 4096,
    .max_address_bits = 48
};

const hal_memory_caps_t *hal_memory_caps(void) {
    return &riscv64_memory_caps;
}
