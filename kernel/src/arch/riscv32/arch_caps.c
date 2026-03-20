#include <arch/arch_caps.h>
#include <arch/arch_cap_profile.h>

arch_caps_t arch_get_caps(void) {
    arch_caps_t caps = {0};

    // EDGE32 Tier 2 - minimal profile
    arch_caps_set(&caps, ARCH_CAP_MMU_FULL);

    return caps;
}

const arch_cap_profile_t *arch_get_cap_profile(void) {
    static const arch_cap_profile_t profile = {
        .tier = ARCH_RUNTIME_TIER2_EDGE32,
        .required = { .bits =
            ARCH_CAP_BIT(ARCH_CAP_MMU_FULL)
        },
        .optional = { .bits =
            ARCH_CAP_BIT(ARCH_CAP_SMP) |
            ARCH_CAP_BIT(ARCH_CAP_DMA_COHERENT) |
            ARCH_CAP_BIT(ARCH_CAP_ADV_IRQ_ROUTING) |
            ARCH_CAP_BIT(ARCH_CAP_USERSPACE_HIGHHALF) |
            ARCH_CAP_BIT(ARCH_CAP_HW_CRC) |
            ARCH_CAP_BIT(ARCH_CAP_SIMD_NET_CSUM)
        }
    };
    return &profile;
}
