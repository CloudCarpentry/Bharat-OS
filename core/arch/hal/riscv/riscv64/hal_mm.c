#include <stdbool.h>
#include "hal/hal_capabilities.h"
#include "hal/hal_mmu.h"
#include <string.h>

int hal_mem_get_caps(hal_mem_caps_t *caps) {
    if (!caps) return -1;

    memset(caps, 0, sizeof(hal_mem_caps_t));

    caps->model = HAL_MEMORY_MODEL_MMU_FULL;
    caps->va_bits = 39; // Sv39 (most common for riscv64)
    caps->pa_bits = 56;
    caps->page_sizes_mask = HAL_PAGE_SIZE_4K | HAL_PAGE_SIZE_2M | HAL_PAGE_SIZE_1G;

    caps->supports_nx = true;
    caps->supports_asid = true;
    caps->supports_global = true;
    caps->supports_user_mode = true;
    caps->supports_write_protect = true;
    caps->supports_hugepages = true;
    caps->supports_dirty_accessed = true;
    caps->supports_iommu = true; // IOMMU

    return 0;
}

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
    .has_iommu = true,
    .has_cache_ops = true,
    .has_tlb_ops = true,
    .max_supported_cores = 32
};

const hal_arch_capabilities_t *hal_get_arch_capabilities_riscv64(void) {
    return &g_riscv64_caps;
}
