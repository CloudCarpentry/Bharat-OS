#include "hal/hal_hw_caps.h"
#include "hal/hal_internal.h"
#include <string.h>

void arch_discover_hw_caps(void) {
    hal_hw_caps_t caps = {0};

    // RISC-V 64 typical capabilities
    caps.has_mmu = true;
    caps.has_mpu = true; // PMP
    caps.has_iommu = true;
    caps.has_dma_coherent = false;
    caps.has_cache_maintenance = true;
    caps.has_local_apic_or_gic_or_plic = true; // PLIC/CLIC
    caps.has_high_res_timer = true;
    caps.has_atomic_64 = true;
    caps.has_vector = true;
    caps.has_crypto_accel = false;
    caps.has_accel_device = false;
    caps.max_cpus = 64;
    caps.page_granule = 4096;
    caps.cache_line_size = 64;

    hal_set_internal_hw_caps(&caps);
}
