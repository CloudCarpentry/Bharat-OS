#include "arch/arch_cpu_caps.h"
#include "../../common/cpu_caps_state.h"
#include <stdint.h>

extern uint32_t hal_cpu_get_id(void);

static void arm32_probe_caps(arch_cpu_caps_record_t *caps) {
    arch_cpu_caps_zero(&caps->raw);
    arch_cpu_caps_zero(&caps->usable);

    /*
     * ARM32 has widely-varying feature discoverability depending on privilege
     * level and CPU generation. For now we advertise a deterministic baseline
     * capability set that is safe for generic ARMv7-A class builds.
     */
    arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);
    arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_VECTOR);
    arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_VECTOR);
#endif

#if defined(__ARM_FEATURE_CRYPTO)
    arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_CRYPTO);
    arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_CRYPTO);
    arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_AES);
    arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_AES);
    arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_SHA);
    arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_SHA);
#endif
}

void arch_cpu_caps_init(void) {
    arch_cpu_caps_record_t boot_caps;
    arm32_probe_caps(&boot_caps);
    cpu_caps_state_set_boot(&boot_caps);
}

void arch_cpu_caps_init_ap(void) {
    arch_cpu_caps_record_t ap_caps;
    arm32_probe_caps(&ap_caps);
    cpu_caps_state_set_ap(hal_cpu_get_id(), &ap_caps);
}
