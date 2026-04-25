#include "arch/common/accel_caps_publish.h"

#include <stdint.h>

static inline void accel_set_mask(uint64_t *mask, hal_accel_feature_t feat,
                                  const arch_cpu_caps_t *caps, int arch_feat) {
    if (feat >= HAL_ACCEL_FEAT__COUNT) {
        return;
    }
    if (arch_cpu_caps_test(caps, arch_feat)) {
        *mask |= (1ULL << (uint32_t)feat);
    }
}

void arch_accel_caps_publish_target(accel_discovery_t *accel,
                                    const arch_cpu_caps_record_t *caps_all,
                                    const arch_cpu_caps_record_t *caps_any) {
    if (!accel || !caps_all || !caps_any) {
        return;
    }

#if defined(__x86_64__)
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
#elif defined(__aarch64__)
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
#elif defined(__riscv)
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
#endif
}
