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

    arch_cpu_caps_set(&boot.raw, ARCH_CPU_FEAT_COMMON_AES);
    arch_cpu_caps_set(&boot.usable, ARCH_CPU_FEAT_COMMON_AES);
    arch_cpu_caps_set(&boot.raw, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);
    arch_cpu_caps_set(&boot.usable, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);

    arch_cpu_caps_set(&ap1.raw, ARCH_CPU_FEAT_COMMON_SHA);
    arch_cpu_caps_set(&ap1.usable, ARCH_CPU_FEAT_COMMON_SHA);
    arch_cpu_caps_set(&ap1.raw, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);
    arch_cpu_caps_set(&ap1.usable, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);

    cpu_caps_state_set_boot(&boot);
    cpu_caps_state_set_ap(1, &ap1);
    arch_cpu_caps_system_finalize();

    if (expect_true(!arch_cpu_caps_test(&arch_cpu_caps_system_all()->usable, ARCH_CPU_FEAT_COMMON_AES),
                    "system_all must clear features missing on AP")) return 1;
    if (expect_true(arch_cpu_caps_test(&arch_cpu_caps_system_any()->usable, ARCH_CPU_FEAT_COMMON_AES),
                    "system_any must include AES from boot CPU")) return 1;
    if (expect_true(arch_cpu_caps_test(&arch_cpu_caps_system_any()->usable, ARCH_CPU_FEAT_COMMON_SHA),
                    "system_any must include SHA from AP")) return 1;

    if (expect_true(hal_cpu_has_feature(0, HAL_CPU_FEATURE_AES),
                    "CPU0 HAL feature view should expose AES")) return 1;
    if (expect_true(hal_cpu_has_feature(1, HAL_CPU_FEATURE_SHA),
                    "CPU1 HAL feature view should expose SHA")) return 1;
    if (expect_true(!hal_cpu_has_system_feature(HAL_CPU_FEATURE_AES, HAL_CPU_FEATURE_SCOPE_ALL),
                    "system ALL must not expose AES")) return 1;
    if (expect_true(hal_cpu_has_system_feature(HAL_CPU_FEATURE_AES, HAL_CPU_FEATURE_SCOPE_ANY),
                    "system ANY must expose AES")) return 1;

    g_fake_cpu_id = 1;
    const arch_cpu_caps_record_t *current = arch_cpu_caps_current();
    if (expect_true(current != NULL, "current cpu record should be available")) return 1;
    if (expect_true(arch_cpu_caps_test(&current->usable, ARCH_CPU_FEAT_COMMON_SHA),
                    "current cpu record should map to AP caps")) return 1;

    printf("PASS: hal cpu feature normalization + aggregation\n");
    return 0;
}
