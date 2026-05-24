#include <stdbool.h>
#include "hal/hal_capabilities.h"
#include "hal/hal_mmu.h"
#include "hal/hal_mm.h"
#include <string.h>

int hal_mem_get_caps(hal_mem_caps_t *caps) {
    if (!caps) return -1;

    memset(caps, 0, sizeof(hal_mem_caps_t));

    caps->model = HAL_MEMORY_MODEL_MMU_LITE;
    caps->va_bits = 32; // Sv32
    caps->pa_bits = 34;
    caps->page_sizes_mask = HAL_PAGE_SIZE_4K | HAL_PAGE_SIZE_4M;

    caps->supports_nx = true;
    caps->supports_execute_never = true;
    caps->supports_user_no_exec = false;
    caps->supports_asid = true;
    caps->supports_global = true;
    caps->supports_user_mode = true;
    caps->supports_user_kernel_isolation = true;
    caps->supports_write_protect = true;

    caps->supports_hugepages = true;
    caps->supports_dirty_accessed = true;
    caps->supports_iommu = false;
    caps->supports_tlb_shootdown = false;
    caps->supports_range_invalidate = false;
    caps->supports_dma_mapping = false;
    caps->supports_guard_pages = false;
    caps->supports_copy_user_validation = false;

    caps->page_table_levels = 2;

    return 0;
}

static const hal_arch_capabilities_t g_riscv32_caps = {
    .arch_name = "riscv32",
    .arch_bits = 32,
    .support_level = BH_ARCH_SUPPORT_BOOT_SUPPORTED,
    .memory_model = BH_MEMORY_MODEL_MMU_LITE,
    .has_smp = false,
    .has_irq_controller = true,
    .has_monotonic_timer = true,
    .has_cycle_counter = true,
    .has_dma = false,
    .has_iommu = false,
    .has_cache_ops = true,
    .has_tlb_ops = true,
    .max_supported_cores = 1
};

const hal_arch_capabilities_t *hal_get_arch_capabilities_riscv32(void) {
    return &g_riscv32_caps;
}

static const hal_memory_caps_t riscv32_memory_caps = {
    .supports_mmu_full = false,
    .supports_mmu_lite = true,
    .supports_mpu_only = false,
    .supports_user_kernel_split = true,
    .supports_page_protection = true,
    .supports_execute_disable = true,
    .supports_asid = true,
    .supports_range_tlb_flush = true,
    .min_page_size = 4096,
    .max_address_bits = 32
};

const hal_memory_caps_t *hal_memory_caps(void) {
    return &riscv32_memory_caps;
}
