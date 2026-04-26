#include "arch/common/accel_caps_publish.h"
#include <stdint.h>

void arch_accel_caps_publish_target_impl(accel_discovery_t *accel,
                                    const arch_cpu_caps_record_t *caps_all,
                                    const arch_cpu_caps_record_t *caps_any) {
    accel_set_mask(&accel->raw_all_mask, HAL_ACCEL_FEAT_RISCV_V,
                   &caps_all->raw, ARCH_CPU_FEAT_RISCV_V);
    accel_set_mask(&accel->raw_any_mask, HAL_ACCEL_FEAT_RISCV_V,
                   &caps_any->raw, ARCH_CPU_FEAT_RISCV_V);
    accel_set_mask(&accel->usable_all_mask, HAL_ACCEL_FEAT_RISCV_V,
                   &caps_all->usable, ARCH_CPU_FEAT_RISCV_V);
    accel_set_mask(&accel->usable_any_mask, HAL_ACCEL_FEAT_RISCV_V,
                   &caps_any->usable, ARCH_CPU_FEAT_RISCV_V);

    accel_set_mask(&accel->raw_all_mask, HAL_ACCEL_FEAT_RISCV_ZBA,
                   &caps_all->raw, ARCH_CPU_FEAT_RISCV_ZBA);
    accel_set_mask(&accel->raw_any_mask, HAL_ACCEL_FEAT_RISCV_ZBA,
                   &caps_any->raw, ARCH_CPU_FEAT_RISCV_ZBA);
    accel_set_mask(&accel->usable_all_mask, HAL_ACCEL_FEAT_RISCV_ZBA,
                   &caps_all->usable, ARCH_CPU_FEAT_RISCV_ZBA);
    accel_set_mask(&accel->usable_any_mask, HAL_ACCEL_FEAT_RISCV_ZBA,
                   &caps_any->usable, ARCH_CPU_FEAT_RISCV_ZBA);

    accel_set_mask(&accel->raw_all_mask, HAL_ACCEL_FEAT_RISCV_ZBB,
                   &caps_all->raw, ARCH_CPU_FEAT_RISCV_ZBB);
    accel_set_mask(&accel->raw_any_mask, HAL_ACCEL_FEAT_RISCV_ZBB,
                   &caps_any->raw, ARCH_CPU_FEAT_RISCV_ZBB);
    accel_set_mask(&accel->usable_all_mask, HAL_ACCEL_FEAT_RISCV_ZBB,
                   &caps_all->usable, ARCH_CPU_FEAT_RISCV_ZBB);
    accel_set_mask(&accel->usable_any_mask, HAL_ACCEL_FEAT_RISCV_ZBB,
                   &caps_any->usable, ARCH_CPU_FEAT_RISCV_ZBB);

    accel_set_mask(&accel->raw_all_mask, HAL_ACCEL_FEAT_RISCV_ZBC,
                   &caps_all->raw, ARCH_CPU_FEAT_RISCV_ZBC);
    accel_set_mask(&accel->raw_any_mask, HAL_ACCEL_FEAT_RISCV_ZBC,
                   &caps_any->raw, ARCH_CPU_FEAT_RISCV_ZBC);
    accel_set_mask(&accel->usable_all_mask, HAL_ACCEL_FEAT_RISCV_ZBC,
                   &caps_all->usable, ARCH_CPU_FEAT_RISCV_ZBC);
    accel_set_mask(&accel->usable_any_mask, HAL_ACCEL_FEAT_RISCV_ZBC,
                   &caps_any->usable, ARCH_CPU_FEAT_RISCV_ZBC);

    accel_set_mask(&accel->raw_all_mask, HAL_ACCEL_FEAT_RISCV_ZBS,
                   &caps_all->raw, ARCH_CPU_FEAT_RISCV_ZBS);
    accel_set_mask(&accel->raw_any_mask, HAL_ACCEL_FEAT_RISCV_ZBS,
                   &caps_any->raw, ARCH_CPU_FEAT_RISCV_ZBS);
    accel_set_mask(&accel->usable_all_mask, HAL_ACCEL_FEAT_RISCV_ZBS,
                   &caps_all->usable, ARCH_CPU_FEAT_RISCV_ZBS);
    accel_set_mask(&accel->usable_any_mask, HAL_ACCEL_FEAT_RISCV_ZBS,
                   &caps_any->usable, ARCH_CPU_FEAT_RISCV_ZBS);
}
