#include <arch/arch_caps.h>
#include <arch/arch_cap_profile.h>
#include <profile/profile.h>

arch_caps_t arch_get_caps(void) {
    arch_caps_t caps = {0};

    /* QEMU virt uses the Sv32 MMU-Lite backend and is UP-first. */
    arch_caps_set(&caps, ARCH_CAP_MMU_LITE);
    arch_caps_set(&caps, ARCH_CAP_CACHE_MAINTENANCE);
    arch_caps_set(&caps, ARCH_CAP_DEVICE_MEMORY_ATTRS);

    return caps;
}

const arch_cap_profile_t *arch_get_cap_profile(void) {
    static const arch_cap_profile_t profile = {
        .tier = ARCH_RUNTIME_TIER2_EDGE32,
        .required = { .bits =
            ARCH_CAP_BIT(ARCH_CAP_MMU_LITE) |
            ARCH_CAP_BIT(ARCH_CAP_CACHE_MAINTENANCE)
        },
        .optional = { .bits =
            ARCH_CAP_BIT(ARCH_CAP_ADV_IRQ_ROUTING)
        }
    };
    return &profile;
}
