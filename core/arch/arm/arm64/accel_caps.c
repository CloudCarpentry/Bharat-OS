#include "arch/common/accel_caps_publish.h"
#include <stdint.h>

void arch_accel_caps_publish_target_impl(accel_discovery_t *accel,
                                    const arch_cpu_caps_record_t *caps_all,
                                    const arch_cpu_caps_record_t *caps_any) {
    accel_set_mask(&accel->raw_all_mask, HAL_ACCEL_FEAT_ARM64_SVE,
                   &caps_all->raw, ARCH_CPU_FEAT_ARM64_SVE);
    accel_set_mask(&accel->raw_any_mask, HAL_ACCEL_FEAT_ARM64_SVE,
                   &caps_any->raw, ARCH_CPU_FEAT_ARM64_SVE);
    accel_set_mask(&accel->usable_all_mask, HAL_ACCEL_FEAT_ARM64_SVE,
                   &caps_all->usable, ARCH_CPU_FEAT_ARM64_SVE);
    accel_set_mask(&accel->usable_any_mask, HAL_ACCEL_FEAT_ARM64_SVE,
                   &caps_any->usable, ARCH_CPU_FEAT_ARM64_SVE);

    accel_set_mask(&accel->raw_all_mask, HAL_ACCEL_FEAT_ARM64_SVE2,
                   &caps_all->raw, ARCH_CPU_FEAT_ARM64_SVE2);
    accel_set_mask(&accel->raw_any_mask, HAL_ACCEL_FEAT_ARM64_SVE2,
                   &caps_any->raw, ARCH_CPU_FEAT_ARM64_SVE2);
    accel_set_mask(&accel->usable_all_mask, HAL_ACCEL_FEAT_ARM64_SVE2,
                   &caps_all->usable, ARCH_CPU_FEAT_ARM64_SVE2);
    accel_set_mask(&accel->usable_any_mask, HAL_ACCEL_FEAT_ARM64_SVE2,
                   &caps_any->usable, ARCH_CPU_FEAT_ARM64_SVE2);
}
