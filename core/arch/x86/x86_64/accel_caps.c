#include "arch/common/accel_caps_publish.h"
#include <stdint.h>

void arch_accel_caps_publish_target_impl(accel_discovery_t *accel,
                                    const arch_cpu_caps_record_t *caps_all,
                                    const arch_cpu_caps_record_t *caps_any) {
    accel_set_mask(&accel->raw_all_mask, HAL_ACCEL_FEAT_X86_AVX,
                   &caps_all->raw, ARCH_CPU_FEAT_X86_AVX);
    accel_set_mask(&accel->raw_any_mask, HAL_ACCEL_FEAT_X86_AVX,
                   &caps_any->raw, ARCH_CPU_FEAT_X86_AVX);
    accel_set_mask(&accel->usable_all_mask, HAL_ACCEL_FEAT_X86_AVX,
                   &caps_all->usable, ARCH_CPU_FEAT_X86_AVX);
    accel_set_mask(&accel->usable_any_mask, HAL_ACCEL_FEAT_X86_AVX,
                   &caps_any->usable, ARCH_CPU_FEAT_X86_AVX);

    accel_set_mask(&accel->raw_all_mask, HAL_ACCEL_FEAT_X86_AVX2,
                   &caps_all->raw, ARCH_CPU_FEAT_X86_AVX2);
    accel_set_mask(&accel->raw_any_mask, HAL_ACCEL_FEAT_X86_AVX2,
                   &caps_any->raw, ARCH_CPU_FEAT_X86_AVX2);
    accel_set_mask(&accel->usable_all_mask, HAL_ACCEL_FEAT_X86_AVX2,
                   &caps_all->usable, ARCH_CPU_FEAT_X86_AVX2);
    accel_set_mask(&accel->usable_any_mask, HAL_ACCEL_FEAT_X86_AVX2,
                   &caps_any->usable, ARCH_CPU_FEAT_X86_AVX2);

    accel_set_mask(&accel->raw_all_mask, HAL_ACCEL_FEAT_X86_FMA,
                   &caps_all->raw, ARCH_CPU_FEAT_X86_FMA);
    accel_set_mask(&accel->raw_any_mask, HAL_ACCEL_FEAT_X86_FMA,
                   &caps_any->raw, ARCH_CPU_FEAT_X86_FMA);
    accel_set_mask(&accel->usable_all_mask, HAL_ACCEL_FEAT_X86_FMA,
                   &caps_all->usable, ARCH_CPU_FEAT_X86_FMA);
    accel_set_mask(&accel->usable_any_mask, HAL_ACCEL_FEAT_X86_FMA,
                   &caps_any->usable, ARCH_CPU_FEAT_X86_FMA);

    accel_set_mask(&accel->raw_all_mask, HAL_ACCEL_FEAT_X86_PCLMUL,
                   &caps_all->raw, ARCH_CPU_FEAT_X86_PCLMUL);
    accel_set_mask(&accel->raw_any_mask, HAL_ACCEL_FEAT_X86_PCLMUL,
                   &caps_any->raw, ARCH_CPU_FEAT_X86_PCLMUL);
    accel_set_mask(&accel->usable_all_mask, HAL_ACCEL_FEAT_X86_PCLMUL,
                   &caps_all->usable, ARCH_CPU_FEAT_X86_PCLMUL);
    accel_set_mask(&accel->usable_any_mask, HAL_ACCEL_FEAT_X86_PCLMUL,
                   &caps_any->usable, ARCH_CPU_FEAT_X86_PCLMUL);
}
