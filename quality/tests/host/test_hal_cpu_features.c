#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "arch/arch_cpu_caps.h"
#include "arch/common/cpu_caps_state.h"
#include "hal/hal_cpu_features.h"

static uint32_t g_fake_cpu_id;

uint32_t hal_cpu_get_id(void) {
    return g_fake_cpu_id;
}

static int expect_true(bool cond, const char *msg) {
    if (!cond) {
        printf("FAIL: %s\n", msg);
        return 1;
    }
    return 0;
}

int main(void) {
    arch_cpu_caps_record_t boot = {0};
    arch_cpu_caps_record_t ap1 = {0};
    arch_cpu_caps_record_t ap2 = {0};

    /*
     * Heterogeneous 3-core cluster setup:
     * - CPU 0: AES, PMULL, CRC32C, STRONG_ATOMICS
     * - CPU 1: AES, SHA, CRC32C, STRONG_ATOMICS
     * - CPU 2: SHA, CRC32, CRC32C, STRONG_ATOMICS
     */
    arch_cpu_caps_set(&boot.raw, ARCH_CPU_FEAT_COMMON_AES);
    arch_cpu_caps_set(&boot.usable, ARCH_CPU_FEAT_COMMON_AES);
    arch_cpu_caps_set(&boot.raw, ARCH_CPU_FEAT_COMMON_PMULL);
    arch_cpu_caps_set(&boot.usable, ARCH_CPU_FEAT_COMMON_PMULL);
    arch_cpu_caps_set(&boot.raw, ARCH_CPU_FEAT_COMMON_CRC32C);
    arch_cpu_caps_set(&boot.usable, ARCH_CPU_FEAT_COMMON_CRC32C);
    arch_cpu_caps_set(&boot.raw, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);
    arch_cpu_caps_set(&boot.usable, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);

    /* CPU 0 raw has VECTOR, but usable clears it (e.g., OS vector policy disabled) */
    arch_cpu_caps_set(&boot.raw, ARCH_CPU_FEAT_COMMON_VECTOR);

    arch_cpu_caps_set(&ap1.raw, ARCH_CPU_FEAT_COMMON_AES);
    arch_cpu_caps_set(&ap1.usable, ARCH_CPU_FEAT_COMMON_AES);
    arch_cpu_caps_set(&ap1.raw, ARCH_CPU_FEAT_COMMON_SHA);
    arch_cpu_caps_set(&ap1.usable, ARCH_CPU_FEAT_COMMON_SHA);
    arch_cpu_caps_set(&ap1.raw, ARCH_CPU_FEAT_COMMON_CRC32C);
    arch_cpu_caps_set(&ap1.usable, ARCH_CPU_FEAT_COMMON_CRC32C);
    arch_cpu_caps_set(&ap1.raw, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);
    arch_cpu_caps_set(&ap1.usable, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);

    arch_cpu_caps_set(&ap2.raw, ARCH_CPU_FEAT_COMMON_SHA);
    arch_cpu_caps_set(&ap2.usable, ARCH_CPU_FEAT_COMMON_SHA);
    arch_cpu_caps_set(&ap2.raw, ARCH_CPU_FEAT_COMMON_CRC32);
    arch_cpu_caps_set(&ap2.usable, ARCH_CPU_FEAT_COMMON_CRC32);
    arch_cpu_caps_set(&ap2.raw, ARCH_CPU_FEAT_COMMON_CRC32C);
    arch_cpu_caps_set(&ap2.usable, ARCH_CPU_FEAT_COMMON_CRC32C);
    arch_cpu_caps_set(&ap2.raw, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);
    arch_cpu_caps_set(&ap2.usable, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);

    cpu_caps_state_set_boot(&boot);
    cpu_caps_state_set_ap(1, &ap1);
    cpu_caps_state_set_ap(2, &ap2);
    arch_cpu_caps_system_finalize();

    hal_cpu_feature_set_t late = {0};
    if (expect_true(hal_cpu_features_is_frozen(), "HAL features must freeze after aggregation")) return 1;
    if (expect_true(!hal_cpu_features_publish(3, &late), "late feature publication must fail")) return 1;

    /* 1. Raw vs Usable Gating Test */
    if (expect_true(!hal_cpu_has_feature(0, HAL_CPU_FEATURE_VECTOR),
                    "CPU0 must NOT report VECTOR as usable when OS policy cleared it")) return 1;

    /* 2. System ALL (Intersection) Tests */
    if (expect_true(!arch_cpu_caps_test(&arch_cpu_caps_system_all()->usable, ARCH_CPU_FEAT_COMMON_AES),
                    "system_all must clear AES (missing on CPU2)")) return 1;
    if (expect_true(!arch_cpu_caps_test(&arch_cpu_caps_system_all()->usable, ARCH_CPU_FEAT_COMMON_SHA),
                    "system_all must clear SHA (missing on CPU0)")) return 1;
    if (expect_true(!arch_cpu_caps_test(&arch_cpu_caps_system_all()->usable, ARCH_CPU_FEAT_COMMON_PMULL),
                    "system_all must clear PMULL (present only on CPU0)")) return 1;
    if (expect_true(!arch_cpu_caps_test(&arch_cpu_caps_system_all()->usable, ARCH_CPU_FEAT_COMMON_CRC32),
                    "system_all must clear CRC32 (present only on CPU2)")) return 1;
    if (expect_true(arch_cpu_caps_test(&arch_cpu_caps_system_all()->usable, ARCH_CPU_FEAT_COMMON_CRC32C),
                    "system_all must preserve CRC32C (present on all CPUs)")) return 1;
    if (expect_true(arch_cpu_caps_test(&arch_cpu_caps_system_all()->usable, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS),
                    "system_all must preserve STRONG_ATOMICS (present on all CPUs)")) return 1;

    /* 3. System ANY (Union) Tests */
    if (expect_true(arch_cpu_caps_test(&arch_cpu_caps_system_any()->usable, ARCH_CPU_FEAT_COMMON_AES),
                    "system_any must include AES")) return 1;
    if (expect_true(arch_cpu_caps_test(&arch_cpu_caps_system_any()->usable, ARCH_CPU_FEAT_COMMON_SHA),
                    "system_any must include SHA")) return 1;
    if (expect_true(arch_cpu_caps_test(&arch_cpu_caps_system_any()->usable, ARCH_CPU_FEAT_COMMON_PMULL),
                    "system_any must include PMULL")) return 1;
    if (expect_true(arch_cpu_caps_test(&arch_cpu_caps_system_any()->usable, ARCH_CPU_FEAT_COMMON_CRC32),
                    "system_any must include CRC32")) return 1;
    if (expect_true(arch_cpu_caps_test(&arch_cpu_caps_system_any()->usable, ARCH_CPU_FEAT_COMMON_CRC32C),
                    "system_any must include CRC32C")) return 1;

    /* 4. Per-Core HAL Query Tests */
    if (expect_true(hal_cpu_has_feature(0, HAL_CPU_FEATURE_AES),
                    "CPU0 HAL feature view should expose AES")) return 1;
    if (expect_true(hal_cpu_has_feature(0, HAL_CPU_FEATURE_PMULL),
                    "CPU0 HAL feature view should expose PMULL")) return 1;
    if (expect_true(!hal_cpu_has_feature(1, HAL_CPU_FEATURE_PMULL),
                    "CPU1 HAL feature view must not expose PMULL")) return 1;
    if (expect_true(hal_cpu_has_feature(1, HAL_CPU_FEATURE_SHA),
                    "CPU1 HAL feature view should expose SHA")) return 1;
    if (expect_true(hal_cpu_has_feature(2, HAL_CPU_FEATURE_CRC32),
                    "CPU2 HAL feature view should expose CRC32")) return 1;
    if (expect_true(!hal_cpu_has_feature(0, HAL_CPU_FEATURE_CRC32),
                    "CPU0 HAL feature view must not expose CRC32")) return 1;
    if (expect_true(hal_cpu_has_feature(0, HAL_CPU_FEATURE_CRC32C),
                    "CPU0 HAL feature view should expose CRC32C")) return 1;
    if (expect_true(hal_cpu_has_feature(1, HAL_CPU_FEATURE_CRC32C),
                    "CPU1 HAL feature view should expose CRC32C")) return 1;
    if (expect_true(hal_cpu_has_feature(2, HAL_CPU_FEATURE_CRC32C),
                    "CPU2 HAL feature view should expose CRC32C")) return 1;

    /* 5. HAL System Scope Tests */
    if (expect_true(!hal_cpu_has_system_feature(HAL_CPU_FEATURE_AES, HAL_CPU_FEATURE_SCOPE_ALL),
                    "system ALL must not expose AES")) return 1;
    if (expect_true(hal_cpu_has_system_feature(HAL_CPU_FEATURE_AES, HAL_CPU_FEATURE_SCOPE_ANY),
                    "system ANY must expose AES")) return 1;
    if (expect_true(!hal_cpu_has_system_feature(HAL_CPU_FEATURE_CRC32, HAL_CPU_FEATURE_SCOPE_ALL),
                    "system ALL must not expose CRC32")) return 1;
    if (expect_true(hal_cpu_has_system_feature(HAL_CPU_FEATURE_CRC32C, HAL_CPU_FEATURE_SCOPE_ALL),
                    "system ALL must expose CRC32C")) return 1;

    /* 6. Current CPU Queries */
    g_fake_cpu_id = 1;
    const arch_cpu_caps_record_t *current1 = arch_cpu_caps_current();
    if (expect_true(current1 != NULL, "current cpu record should be available")) return 1;
    if (expect_true(arch_cpu_caps_test(&current1->usable, ARCH_CPU_FEAT_COMMON_SHA),
                    "current cpu record should map to CPU1 caps")) return 1;
    if (expect_true(hal_cpu_has_feature_current(HAL_CPU_FEATURE_SHA),
                    "current cpu HAL query should report SHA on CPU1")) return 1;
    if (expect_true(!hal_cpu_has_feature_current(HAL_CPU_FEATURE_PMULL),
                    "current cpu HAL query should NOT report PMULL on CPU1")) return 1;

    /* 7. Bounds / Safe Fallback */
    if (expect_true(!hal_cpu_has_feature(99, HAL_CPU_FEATURE_AES),
                    "unregistered CPU query must return false")) return 1;
    if (expect_true(!hal_cpu_has_feature(0, HAL_CPU_FEATURE__COUNT),
                    "out-of-bounds feature query must return false")) return 1;

    printf("PASS: hal cpu feature normalization + heterogeneous aggregation + CRC separation\n");
    return 0;
}
