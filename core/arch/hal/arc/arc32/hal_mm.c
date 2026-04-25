#include <stdbool.h>
#include "hal/hal_capabilities.h"
#include "hal/hal_mmu.h"
#include <string.h>

int hal_mem_get_caps(hal_mem_caps_t *caps) {
    if (!caps) return -1;

    memset(caps, 0, sizeof(hal_mem_caps_t));

    /* ARC HS3x/HS4x typical MMU configuration */
    caps->model = HAL_MEMORY_MODEL_MMU_FULL;
    caps->va_bits = 32;
    caps->pa_bits = 32; // Can be 40 with PAE, but 32 is standard
    caps->page_sizes_mask = HAL_PAGE_SIZE_4K | HAL_PAGE_SIZE_2M;

    caps->supports_nx = true;
    caps->supports_asid = true;
    caps->supports_global = true;
    caps->supports_user_mode = true;
    caps->supports_write_protect = true;
    caps->supports_hugepages = true;
    caps->supports_dirty_accessed = false; // Usually software managed on ARC
    caps->supports_iommu = false;

    return 0;
}

static const hal_arch_capabilities_t g_arc32_caps = {
    .arch_name = "arc32",
    .arch_bits = 32,
    .support_level = BH_ARCH_SUPPORT_BUILD_SUPPORTED,
    .memory_model = BH_MEMORY_MODEL_MMU_FULL,
    .has_smp = false,
    .has_irq_controller = false,
    .has_monotonic_timer = false,
    .has_cycle_counter = true,
    .has_dma = false,
    .has_iommu = false,
    .has_cache_ops = true,
    .has_tlb_ops = true,
    .max_supported_cores = 1
};

const hal_arch_capabilities_t *hal_get_arch_capabilities(void) {
    return &g_arc32_caps;
}
