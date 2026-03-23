#include <arch/arch_caps.h>
#include <arch/arch_cap_profile.h>
#include <profile.h>

arch_caps_t arch_get_caps(void) {
    arch_caps_t caps = {0};

    // EDGE32 Tier 2 - minimal profile
    if (get_memory_model() == MEM_MODEL_MMU) {
        arch_caps_set(&caps, ARCH_CAP_MMU_LITE);
    } else {
        arch_caps_set(&caps, ARCH_CAP_MPU_ONLY);
    }

    // Most constrained arm32 don't have SMP, ASID, Global TLB, etc.
    arch_caps_set(&caps, ARCH_CAP_CACHE_MAINTENANCE);
    arch_caps_set(&caps, ARCH_CAP_DEVICE_MEMORY_ATTRS);

    return caps;
}

const arch_cap_profile_t *arch_get_cap_profile(void) {
    static const arch_cap_profile_t profile = {
        .tier = ARCH_RUNTIME_TIER2_EDGE32,
        .required = { .bits =
            ARCH_CAP_BIT(ARCH_CAP_CACHE_MAINTENANCE) |
            ARCH_CAP_BIT(ARCH_CAP_DEVICE_MEMORY_ATTRS)
        },
        .optional = { .bits =
            ARCH_CAP_BIT(ARCH_CAP_MMU_LITE) |
            ARCH_CAP_BIT(ARCH_CAP_MPU_ONLY) |
            ARCH_CAP_BIT(ARCH_CAP_SMP) |
            ARCH_CAP_BIT(ARCH_CAP_DMA_COHERENT) |
            ARCH_CAP_BIT(ARCH_CAP_ADV_IRQ_ROUTING)
        }
    };
    return &profile;
}
