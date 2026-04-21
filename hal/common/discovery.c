#include "hal/hal_discovery.h"
#include "hal/hal_cpu_topology.h"
#include "arch/arch_cpu_caps.h"
#include "boot/boot_info.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// Global discovery structure populated by early architecture boot code (ACPI or FDT)
static system_discovery_t g_system_discovery;

system_discovery_t* hal_get_system_discovery(void) {
    return &g_system_discovery;
}

bool hal_cpu_topology_query(hal_cpu_topology_info_t *out) {
    if (!out) {
        return false;
    }

    const system_discovery_t *discovery = hal_get_system_discovery();
    uint32_t discovered = 1U;
    if (discovery && discovery->topology.cpu_count > 0U) {
        discovered = discovery->topology.cpu_count;
    }

    out->discovered_cpu_count = discovered;
    out->smp_available = (discovered > 1U);
    out->homogeneous_cores = true;
    return true;
}

void hal_discovery_init(const boot_info_t *boot) {
    if (!boot) return;
    hal_arch_discovery_init(boot);
}

static inline void accel_set(uint64_t *mask, hal_accel_feature_t feat, bool enabled) {
    if (!enabled || feat >= HAL_ACCEL_FEAT__COUNT) {
        return;
    }
    *mask |= (1ULL << (uint32_t)feat);
}

void hal_discovery_publish_cpu_caps(void) {
    system_discovery_t *sys = hal_get_system_discovery();
    const arch_cpu_caps_record_t *caps_all = arch_cpu_caps_system_all();
    const arch_cpu_caps_record_t *caps_any = arch_cpu_caps_system_any();

    if (caps_all == NULL || caps_any == NULL) {
        sys->accel.raw_all_mask = 0;
        sys->accel.raw_any_mask = 0;
        sys->accel.usable_all_mask = 0;
        sys->accel.usable_any_mask = 0;
        return;
    }

    sys->accel.raw_all_mask = 0;
    sys->accel.raw_any_mask = 0;
    sys->accel.usable_all_mask = 0;
    sys->accel.usable_any_mask = 0;

    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_VECTOR,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_COMMON_VECTOR));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_VECTOR,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_COMMON_VECTOR));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_VECTOR,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_COMMON_VECTOR));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_VECTOR,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_COMMON_VECTOR));

    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_AES,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_COMMON_AES));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_AES,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_COMMON_AES));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_AES,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_COMMON_AES));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_AES,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_COMMON_AES));

    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_SHA,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_COMMON_SHA));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_SHA,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_COMMON_SHA));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_SHA,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_COMMON_SHA));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_SHA,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_COMMON_SHA));

    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_PMULL,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_COMMON_PMULL));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_PMULL,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_COMMON_PMULL));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_PMULL,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_COMMON_PMULL));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_PMULL,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_COMMON_PMULL));

    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_STRONG_ATOMICS,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_STRONG_ATOMICS,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_STRONG_ATOMICS,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_STRONG_ATOMICS,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS));

#if defined(__x86_64__)
    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_X86_AVX,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_X86_AVX));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_X86_AVX,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_X86_AVX));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_X86_AVX,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_X86_AVX));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_X86_AVX,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_X86_AVX));

    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_X86_AVX2,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_X86_AVX2));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_X86_AVX2,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_X86_AVX2));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_X86_AVX2,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_X86_AVX2));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_X86_AVX2,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_X86_AVX2));

    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_X86_FMA,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_X86_FMA));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_X86_FMA,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_X86_FMA));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_X86_FMA,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_X86_FMA));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_X86_FMA,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_X86_FMA));

    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_X86_PCLMUL,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_X86_PCLMUL));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_X86_PCLMUL,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_X86_PCLMUL));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_X86_PCLMUL,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_X86_PCLMUL));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_X86_PCLMUL,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_X86_PCLMUL));
#elif defined(__aarch64__)
    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_ARM64_SVE,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_ARM64_SVE));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_ARM64_SVE,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_ARM64_SVE));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_ARM64_SVE,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_ARM64_SVE));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_ARM64_SVE,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_ARM64_SVE));

    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_ARM64_SVE2,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_ARM64_SVE2));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_ARM64_SVE2,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_ARM64_SVE2));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_ARM64_SVE2,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_ARM64_SVE2));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_ARM64_SVE2,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_ARM64_SVE2));
#elif defined(__riscv)
    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_RISCV_V,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_RISCV_V));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_RISCV_V,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_RISCV_V));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_RISCV_V,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_RISCV_V));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_RISCV_V,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_RISCV_V));

    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_RISCV_ZBA,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_RISCV_ZBA));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_RISCV_ZBA,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_RISCV_ZBA));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_RISCV_ZBA,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_RISCV_ZBA));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_RISCV_ZBA,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_RISCV_ZBA));

    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_RISCV_ZBB,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_RISCV_ZBB));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_RISCV_ZBB,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_RISCV_ZBB));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_RISCV_ZBB,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_RISCV_ZBB));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_RISCV_ZBB,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_RISCV_ZBB));

    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_RISCV_ZBC,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_RISCV_ZBC));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_RISCV_ZBC,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_RISCV_ZBC));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_RISCV_ZBC,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_RISCV_ZBC));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_RISCV_ZBC,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_RISCV_ZBC));

    accel_set(&sys->accel.raw_all_mask, HAL_ACCEL_FEAT_RISCV_ZBS,
              arch_cpu_caps_test(&caps_all->raw, ARCH_CPU_FEAT_RISCV_ZBS));
    accel_set(&sys->accel.raw_any_mask, HAL_ACCEL_FEAT_RISCV_ZBS,
              arch_cpu_caps_test(&caps_any->raw, ARCH_CPU_FEAT_RISCV_ZBS));
    accel_set(&sys->accel.usable_all_mask, HAL_ACCEL_FEAT_RISCV_ZBS,
              arch_cpu_caps_test(&caps_all->usable, ARCH_CPU_FEAT_RISCV_ZBS));
    accel_set(&sys->accel.usable_any_mask, HAL_ACCEL_FEAT_RISCV_ZBS,
              arch_cpu_caps_test(&caps_any->usable, ARCH_CPU_FEAT_RISCV_ZBS));
#endif
}
