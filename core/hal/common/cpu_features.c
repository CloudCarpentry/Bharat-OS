#include "hal/hal_cpu_features.h"

#include "arch/arch_cpu_caps.h"
#include "hal/hal.h"
#include <string.h>

static inline void feature_set_bit(uint64_t *bits, hal_cpu_feature_t feature, bool enabled) {
    if (!enabled || feature >= HAL_CPU_FEATURE__COUNT) {
        return;
    }
    bits[(size_t)feature / 64u] |= (1ULL << ((size_t)feature % 64u));
}

static void map_arch_to_hal(const arch_cpu_caps_record_t *arch, hal_cpu_feature_set_t *out) {
    memset(out, 0, sizeof(*out));
    if (arch == NULL) {
        return;
    }

    feature_set_bit(out->raw_bits, HAL_CPU_FEATURE_VECTOR,
                    arch_cpu_caps_test(&arch->raw, ARCH_CPU_FEAT_COMMON_VECTOR));
    feature_set_bit(out->usable_bits, HAL_CPU_FEATURE_VECTOR,
                    arch_cpu_caps_test(&arch->usable, ARCH_CPU_FEAT_COMMON_VECTOR));

    feature_set_bit(out->raw_bits, HAL_CPU_FEATURE_AES,
                    arch_cpu_caps_test(&arch->raw, ARCH_CPU_FEAT_COMMON_AES));
    feature_set_bit(out->usable_bits, HAL_CPU_FEATURE_AES,
                    arch_cpu_caps_test(&arch->usable, ARCH_CPU_FEAT_COMMON_AES));

    feature_set_bit(out->raw_bits, HAL_CPU_FEATURE_SHA,
                    arch_cpu_caps_test(&arch->raw, ARCH_CPU_FEAT_COMMON_SHA));
    feature_set_bit(out->usable_bits, HAL_CPU_FEATURE_SHA,
                    arch_cpu_caps_test(&arch->usable, ARCH_CPU_FEAT_COMMON_SHA));

    feature_set_bit(out->raw_bits, HAL_CPU_FEATURE_PMULL,
                    arch_cpu_caps_test(&arch->raw, ARCH_CPU_FEAT_COMMON_PMULL));
    feature_set_bit(out->usable_bits, HAL_CPU_FEATURE_PMULL,
                    arch_cpu_caps_test(&arch->usable, ARCH_CPU_FEAT_COMMON_PMULL));

    feature_set_bit(out->raw_bits, HAL_CPU_FEATURE_CRYPTO,
                    arch_cpu_caps_test(&arch->raw, ARCH_CPU_FEAT_COMMON_CRYPTO));
    feature_set_bit(out->usable_bits, HAL_CPU_FEATURE_CRYPTO,
                    arch_cpu_caps_test(&arch->usable, ARCH_CPU_FEAT_COMMON_CRYPTO));

    feature_set_bit(out->raw_bits, HAL_CPU_FEATURE_STRONG_ATOMICS,
                    arch_cpu_caps_test(&arch->raw, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS));
    feature_set_bit(out->usable_bits, HAL_CPU_FEATURE_STRONG_ATOMICS,
                    arch_cpu_caps_test(&arch->usable, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS));

    feature_set_bit(out->raw_bits, HAL_CPU_FEATURE_FAST_TLB_CTX,
                    arch_cpu_caps_test(&arch->raw, ARCH_CPU_FEAT_COMMON_FAST_TLB_CTX));
    feature_set_bit(out->usable_bits, HAL_CPU_FEATURE_FAST_TLB_CTX,
                    arch_cpu_caps_test(&arch->usable, ARCH_CPU_FEAT_COMMON_FAST_TLB_CTX));

#if defined(__aarch64__)
    feature_set_bit(out->raw_bits, HAL_CPU_FEATURE_SCALABLE_VECTOR,
                    arch_cpu_caps_test(&arch->raw, ARCH_CPU_FEAT_ARM64_SVE));
    feature_set_bit(out->usable_bits, HAL_CPU_FEATURE_SCALABLE_VECTOR,
                    arch_cpu_caps_test(&arch->usable, ARCH_CPU_FEAT_ARM64_SVE));
#elif defined(__riscv)
    bool has_any_bitmanip = arch_cpu_caps_test(&arch->raw, ARCH_CPU_FEAT_RISCV_ZBA) ||
                            arch_cpu_caps_test(&arch->raw, ARCH_CPU_FEAT_RISCV_ZBB) ||
                            arch_cpu_caps_test(&arch->raw, ARCH_CPU_FEAT_RISCV_ZBC) ||
                            arch_cpu_caps_test(&arch->raw, ARCH_CPU_FEAT_RISCV_ZBS);
    bool has_any_bitmanip_usable = arch_cpu_caps_test(&arch->usable, ARCH_CPU_FEAT_RISCV_ZBA) ||
                                   arch_cpu_caps_test(&arch->usable, ARCH_CPU_FEAT_RISCV_ZBB) ||
                                   arch_cpu_caps_test(&arch->usable, ARCH_CPU_FEAT_RISCV_ZBC) ||
                                   arch_cpu_caps_test(&arch->usable, ARCH_CPU_FEAT_RISCV_ZBS);
    feature_set_bit(out->raw_bits, HAL_CPU_FEATURE_BITMANIP, has_any_bitmanip);
    feature_set_bit(out->usable_bits, HAL_CPU_FEATURE_BITMANIP, has_any_bitmanip_usable);
#endif
}

bool hal_cpu_feature_set_for_cpu(size_t cpu_id, hal_cpu_feature_set_t *out) {
    if (out == NULL) {
        return false;
    }
    const arch_cpu_caps_record_t *arch = arch_cpu_caps_for_cpu(cpu_id);
    if (arch == NULL) {
        memset(out, 0, sizeof(*out));
        return false;
    }
    map_arch_to_hal(arch, out);
    return true;
}

bool hal_cpu_feature_set_system(hal_cpu_feature_scope_t scope, hal_cpu_feature_set_t *out) {
    if (out == NULL) {
        return false;
    }
    const arch_cpu_caps_record_t *arch = (scope == HAL_CPU_FEATURE_SCOPE_ANY)
                                             ? arch_cpu_caps_system_any()
                                             : arch_cpu_caps_system_all();
    map_arch_to_hal(arch, out);
    return true;
}

bool hal_cpu_has_feature(size_t cpu_id, hal_cpu_feature_t feature) {
    hal_cpu_feature_set_t set;
    if (!hal_cpu_feature_set_for_cpu(cpu_id, &set) || feature >= HAL_CPU_FEATURE__COUNT) {
        return false;
    }
    return (set.usable_bits[(size_t)feature / 64u] & (1ULL << ((size_t)feature % 64u))) != 0;
}

bool hal_cpu_has_system_feature(hal_cpu_feature_t feature, hal_cpu_feature_scope_t scope) {
    if (scope == HAL_CPU_FEATURE_SCOPE_ANY) {
        return hal_cpu_has_system_feature_any(feature);
    }
    return hal_cpu_has_system_feature_all(feature);
}

bool hal_cpu_has_feature_current(hal_cpu_feature_t feature) {
    return hal_cpu_has_feature((size_t)hal_cpu_get_id(), feature);
}

bool hal_cpu_has_system_feature_all(hal_cpu_feature_t feature) {
    hal_cpu_feature_set_t set;
    if (!hal_cpu_feature_set_system(HAL_CPU_FEATURE_SCOPE_ALL, &set) ||
        feature >= HAL_CPU_FEATURE__COUNT) {
        return false;
    }
    return (set.usable_bits[(size_t)feature / 64u] & (1ULL << ((size_t)feature % 64u))) != 0;
}

bool hal_cpu_has_system_feature_any(hal_cpu_feature_t feature) {
    hal_cpu_feature_set_t set;
    if (!hal_cpu_feature_set_system(HAL_CPU_FEATURE_SCOPE_ANY, &set) ||
        feature >= HAL_CPU_FEATURE__COUNT) {
        return false;
    }
    return (set.usable_bits[(size_t)feature / 64u] & (1ULL << ((size_t)feature % 64u))) != 0;
}
