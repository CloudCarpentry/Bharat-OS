#include "hal/hal_hw_caps.h"
#include "hal/hal_internal.h"
#include <string.h>

void arch_discover_hw_caps(void) {
    hal_hw_caps_t caps = {0};

    // RISC-V 32 typical capabilities
    caps.has_mmu = true;
    caps.has_mpu = true; // PMP
    caps.has_iommu = false;
    caps.has_dma_coherent = false;
    caps.has_cache_maintenance = true;
    caps.has_local_apic_or_gic_or_plic = true;
    caps.has_high_res_timer = true;
    caps.has_atomic_64 = false;
    caps.has_vector = false;
    caps.has_crypto_accel = false;
    caps.has_accel_device = false;
    caps.max_cpus = 4;
    caps.page_granule = 4096;
    caps.cache_line_size = 32;

    hal_set_internal_hw_caps(&caps);
}
