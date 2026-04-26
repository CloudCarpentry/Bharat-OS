#include <stdbool.h>
#include "hal/hal_capabilities.h"
#include "hal/hal_mmu.h"
#include <string.h>

int hal_mem_get_caps(hal_mem_caps_t *caps) {
    if (!caps) return -1;
    memset(caps, 0, sizeof(hal_mem_caps_t));

    /* Renesas RX typical configuration (MPU based) */
    caps->model = HAL_MEMORY_MODEL_MPU;
    caps->va_bits = 32;
    caps->pa_bits = 32;
    caps->max_mpu_regions = 8; // RX600/700 series typical

    caps->supports_nx = true;
    caps->supports_user_mode = true;
    caps->supports_write_protect = true;

    return 0;
}

static const hal_arch_capabilities_t g_rx_caps = {
    .arch_name = "rx",
    .arch_bits = 32,
    .support_level = BH_ARCH_SUPPORT_SCAFFOLD_ONLY,
    .memory_model = BH_MEMORY_MODEL_MPU_ONLY,
    .has_smp = false,
    .has_irq_controller = false,
    .has_monotonic_timer = false,
    .has_cycle_counter = false,
    .has_dma = false,
    .has_iommu = false,
    .has_cache_ops = false,
    .has_tlb_ops = false,
    .max_supported_cores = 1
};

const hal_arch_capabilities_t *hal_get_arch_capabilities(void) {
    return &g_rx_caps;
}
