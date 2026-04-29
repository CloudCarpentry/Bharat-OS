#include "hal/hal_hw_caps.h"
#include "kernel/primitive.h"
#include "kernel/status.h"
#include "profile/profile_policy.h"

static hal_hw_caps_t g_discovered_caps;
static bool g_registry_initialized = false;

kstatus_t bh_kernel_primitive_registry_init(const hal_hw_caps_t *caps) {
    if (!caps) {
        return K_ERR_INVALID_ARG;
    }
    g_discovered_caps = *caps;
    g_registry_initialized = true;
    return K_OK;
}

bh_primitive_support_level_t bh_kernel_primitive_get_support_level(bh_kernel_primitive_class_t primitive) {
    if (!g_registry_initialized) {
        return BH_PRIMITIVE_UNSUPPORTED;
    }

    switch (primitive) {
        case BH_PRIMITIVE_SCHED:
            return BH_PRIMITIVE_SOFTWARE_FALLBACK; // Core scheduler is software

        case BH_PRIMITIVE_MEMORY:
            if (g_discovered_caps.has_mmu) {
                return BH_PRIMITIVE_HARDWARE_ENFORCED;
            } else if (g_discovered_caps.has_mpu) {
                return BH_PRIMITIVE_HARDWARE_ASSISTED;
            }
            return BH_PRIMITIVE_SOFTWARE_FALLBACK;

        case BH_PRIMITIVE_CAPABILITY:
            return BH_PRIMITIVE_SOFTWARE_FALLBACK;

        case BH_PRIMITIVE_IPC:
            return BH_PRIMITIVE_SOFTWARE_FALLBACK;

        case BH_PRIMITIVE_TIMER:
            if (g_discovered_caps.has_high_res_timer) {
                return BH_PRIMITIVE_HARDWARE_ASSISTED;
            }
            return BH_PRIMITIVE_SOFTWARE_FALLBACK;

        case BH_PRIMITIVE_FAULT:
            return BH_PRIMITIVE_HARDWARE_ENFORCED; // CPU traps

        case BH_PRIMITIVE_DMA:
            if (g_discovered_caps.has_iommu) {
                return BH_PRIMITIVE_HARDWARE_ENFORCED;
            }
            // DMA fallback depends on policy, but level is software fallback if no IOMMU
            return BH_PRIMITIVE_SOFTWARE_FALLBACK;

        case BH_PRIMITIVE_ACCEL:
            if (g_discovered_caps.has_accel_device) {
                return BH_PRIMITIVE_HARDWARE_ASSISTED;
            }
            return BH_PRIMITIVE_UNSUPPORTED;

        case BH_PRIMITIVE_TELEMETRY:
            return BH_PRIMITIVE_SOFTWARE_FALLBACK;

        default:
            return BH_PRIMITIVE_UNSUPPORTED;
    }
}

bool bh_kernel_primitive_available(bh_kernel_primitive_class_t primitive) {
    return bh_kernel_primitive_get_support_level(primitive) != BH_PRIMITIVE_UNSUPPORTED;
}
