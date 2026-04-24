#include <arch/arch_caps.h>
#include <arch/arch_cap_profile.h>
#include <profile/profile.h>

arch_caps_t arch_get_caps(void) {
    arch_caps_t caps = {0};

    // ROBOT Tier 1 - RTOS profile
    arch_caps_set(&caps, ARCH_CAP_MPU_ONLY);
    arch_caps_set(&caps, ARCH_CAP_SMP);
    arch_caps_set(&caps, ARCH_CAP_CACHE_MAINTENANCE);
    arch_caps_set(&caps, ARCH_CAP_DEVICE_MEMORY_ATTRS);
    arch_caps_set(&caps, ARCH_CAP_DMA_COHERENT);

    return caps;
}

const arch_cap_profile_t *arch_get_cap_profile(void) {
    static const arch_cap_profile_t profile = {
        .tier = ARCH_RUNTIME_TIER1_FULL,
        .required = { .bits =
            ARCH_CAP_BIT(ARCH_CAP_MPU_ONLY) |
            ARCH_CAP_BIT(ARCH_CAP_SMP) |
            ARCH_CAP_BIT(ARCH_CAP_DMA_COHERENT)
        },
        .optional = { .bits =
            ARCH_CAP_BIT(ARCH_CAP_MMU_LITE) |
            ARCH_CAP_BIT(ARCH_CAP_CACHE_MAINTENANCE) |
            ARCH_CAP_BIT(ARCH_CAP_ADV_IRQ_ROUTING)
        }
    };
    return &profile;
}
