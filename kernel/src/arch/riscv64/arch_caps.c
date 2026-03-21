#include <arch/arch_caps.h>
#include <arch/arch_cap_profile.h>

arch_caps_t arch_get_caps(void) {
    arch_caps_t caps = {0};

    arch_caps_set(&caps, ARCH_CAP_64BIT_VA);
    arch_caps_set(&caps, ARCH_CAP_SMP);
    arch_caps_set(&caps, ARCH_CAP_MMU_FULL);
    arch_caps_set(&caps, ARCH_CAP_ASID);
    arch_caps_set(&caps, ARCH_CAP_GLOBAL_TLB_INVALIDATE);
    arch_caps_set(&caps, ARCH_CAP_FINE_GRAIN_PROTECT);
    arch_caps_set(&caps, ARCH_CAP_DMA_COHERENT);
    arch_caps_set(&caps, ARCH_CAP_CACHE_MAINTENANCE);
    arch_caps_set(&caps, ARCH_CAP_DEVICE_MEMORY_ATTRS);
    arch_caps_set(&caps, ARCH_CAP_ADV_IRQ_ROUTING);
    arch_caps_set(&caps, ARCH_CAP_USERSPACE_HIGHHALF);

    return caps;
}

const arch_cap_profile_t *arch_get_cap_profile(void) {
    static const arch_cap_profile_t profile = {
        .tier = ARCH_RUNTIME_TIER1_FULL,
        .required = { .bits =
            ARCH_CAP_BIT(ARCH_CAP_64BIT_VA) |
            ARCH_CAP_BIT(ARCH_CAP_SMP) |
            ARCH_CAP_BIT(ARCH_CAP_MMU_FULL) |
            ARCH_CAP_BIT(ARCH_CAP_DMA_COHERENT) |
            ARCH_CAP_BIT(ARCH_CAP_ADV_IRQ_ROUTING) |
            ARCH_CAP_BIT(ARCH_CAP_USERSPACE_HIGHHALF)
        },
        .optional = { .bits =
            ARCH_CAP_BIT(ARCH_CAP_HW_CRC) |
            ARCH_CAP_BIT(ARCH_CAP_SIMD_NET_CSUM)
        }
    };
    return &profile;
}
