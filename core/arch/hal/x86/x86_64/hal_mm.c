#include <stdbool.h>
#include "hal/hal_capabilities.h"
#include "hal/hal_mm.h"
#include "hal/hal_mmu.h"
#include <string.h>

int hal_mem_get_caps(hal_mem_caps_t *caps) {
    if (!caps) return -1;

    memset(caps, 0, sizeof(hal_mem_caps_t));

    caps->model = HAL_MEMORY_MODEL_MMU_FULL;
    caps->va_bits = 48; // Standard 4-level paging
    caps->pa_bits = 46; // Physical address bits
    caps->page_sizes_mask = HAL_PAGE_SIZE_4K | HAL_PAGE_SIZE_2M | HAL_PAGE_SIZE_1G;

    caps->supports_nx = true;
    caps->supports_execute_never = true;
    caps->supports_user_no_exec = true; // SMEP
    caps->supports_asid = true; // PCID
    caps->supports_global = true;
    caps->supports_user_mode = true;
    caps->supports_user_kernel_isolation = true;
    caps->supports_write_protect = true;

    caps->supports_iommu = true;
    caps->supports_hugepages = true;
    caps->supports_dirty_accessed = true;
    caps->supports_tlb_shootdown = true;
    caps->supports_range_invalidate = true;
    caps->supports_dma_mapping = true;
    caps->supports_guard_pages = true;
    caps->supports_copy_user_validation = true;

    caps->page_table_levels = 4;

    return 0;
}

void hal_mm_get_zone_limits(hal_mm_zone_limits_t *out) {
    if (!out) return;
    out->dma_low_start = 0;
    out->dma_low_end = 0x00FFFFFFu;     /* first 16 MiB */
    out->dma32_start = 0;
    out->dma32_end = 0xFFFFFFFFu;       /* first 4 GiB */
    out->normal_start = 0x100000000ULL; /* above 4 GiB */
    out->normal_end = (uintptr_t)-1;
    out->flags = 0;
}

void hal_mm_backend_caps(hal_mm_backend_caps_t *out) {
    if (!out) return;
    out->kind = HAL_MM_BACKEND_MMU_FULL;
    out->map_granule = 4096;
    out->protect_granule = 4096;
    out->alloc_granule = 4096;
    out->max_regions = 0;
    out->flags = 1;
}

static const hal_arch_capabilities_t g_x86_64_caps = {
    .arch_name = "x86_64",
    .arch_bits = 64,
    .support_level = BH_ARCH_SUPPORT_RUNTIME_SUPPORTED,
    .memory_model = BH_MEMORY_MODEL_MMU_FULL,
    .has_smp = true,
    .has_irq_controller = true,
    .has_monotonic_timer = true,
    .has_cycle_counter = true,
    .has_dma = true,
    .has_iommu = true,
    .has_cache_ops = true,
    .has_tlb_ops = true,
    .max_supported_cores = 64
};

const hal_arch_capabilities_t *hal_get_arch_capabilities(void) {
    return &g_x86_64_caps;
}

static const hal_memory_caps_t x86_64_memory_caps = {
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
    return &x86_64_memory_caps;
}
