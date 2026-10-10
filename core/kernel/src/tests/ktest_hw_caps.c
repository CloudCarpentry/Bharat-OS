#include "../../include/tests/ktest.h"
#include "hal/hal_hw_caps.h"
#include "hal/hal_cpu_features.h"
#include "console/console_core.h"
#include "../../include/kernel.h"

#define KPRINT(s) console_write_raw(s, string_length(s))

static int test_hw_caps_sanity(void) {
    KPRINT("Running Hardware Caps Freeze Conformance Tests...\n");

    // We expect hw_caps to be frozen by the time tests run.
    if (!hal_hw_caps_is_frozen()) {
        KPRINT("FAIL: hal_hw_caps_is_frozen returned false, expected true.\n");
        return -1;
    }

    if (hal_hw_caps_state() != HAL_CAPS_FROZEN) {
        KPRINT("FAIL: hal_hw_caps_state is not HAL_CAPS_FROZEN.\n");
        return -1;
    }

    const hal_hw_caps_t *caps = hal_get_internal_hw_caps();
    if (caps == NULL) {
        KPRINT("FAIL: hal_get_internal_hw_caps returned NULL after freeze.\n");
        return -1;
    }

    // Try modifying caps API and expect K_ERR_BAD_STATE
    hal_hw_caps_t mock_caps = {0};
    if (hal_hw_caps_publish_raw(&mock_caps) != K_ERR_BAD_STATE) {
        KPRINT("FAIL: hal_hw_caps_publish_raw did not return K_ERR_BAD_STATE after freeze.\n");
        return -1;
    }

    if (hal_hw_caps_publish_cpu() != K_ERR_BAD_STATE) {
        KPRINT("FAIL: hal_hw_caps_publish_cpu did not return K_ERR_BAD_STATE after freeze.\n");
        return -1;
    }

    if (hal_hw_caps_finalize() != K_ERR_BAD_STATE) {
        KPRINT("FAIL: hal_hw_caps_finalize did not return K_ERR_BAD_STATE after freeze.\n");
        return -1;
    }

    if (!hal_cpu_features_is_frozen()) {
        KPRINT("FAIL: hal_cpu_features_is_frozen returned false, expected true.\n");
        return -1;
    }

    // Synthetic feature aggregation testing
    // We unfreeze cpu features internally just for this mock test
    extern bool hal_cpu_features_begin(size_t (*current_id)(void));
    extern bool hal_cpu_features_publish(size_t cpu_id, const hal_cpu_feature_set_t *features);
    extern bool hal_cpu_features_freeze(void);

    if (hal_cpu_features_begin(NULL)) {
        hal_cpu_feature_set_t cpu0 = {0};
        hal_cpu_feature_set_t cpu1 = {0};

        // CPU 0 usable: AES, VECTOR
        cpu0.usable_bits[HAL_CPU_FEATURE_AES / 64u] |= (1ULL << (HAL_CPU_FEATURE_AES % 64u));
        cpu0.usable_bits[HAL_CPU_FEATURE_VECTOR / 64u] |= (1ULL << (HAL_CPU_FEATURE_VECTOR % 64u));

        // CPU 1 usable: AES, SHA, AND a raw feature that is NOT usable
        cpu1.usable_bits[HAL_CPU_FEATURE_AES / 64u] |= (1ULL << (HAL_CPU_FEATURE_AES % 64u));
        cpu1.usable_bits[HAL_CPU_FEATURE_SHA / 64u] |= (1ULL << (HAL_CPU_FEATURE_SHA % 64u));
        cpu1.raw_bits[HAL_CPU_FEATURE_SCALABLE_VECTOR / 64u] |= (1ULL << (HAL_CPU_FEATURE_SCALABLE_VECTOR % 64u));

        hal_cpu_features_publish(0, &cpu0);
        hal_cpu_features_publish(1, &cpu1);
        hal_cpu_features_freeze();

        hal_cpu_feature_set_t all_features, any_features;
        if (!hal_cpu_feature_set_system(HAL_CPU_FEATURE_SCOPE_ALL, &all_features) ||
            !hal_cpu_feature_set_system(HAL_CPU_FEATURE_SCOPE_ANY, &any_features)) {
            KPRINT("FAIL: Could not retrieve synthetic system features\n");
            return -1;
        }

        // Expected ALL: AES
        if ((all_features.usable_bits[HAL_CPU_FEATURE_AES / 64u] & (1ULL << (HAL_CPU_FEATURE_AES % 64u))) == 0) {
            KPRINT("FAIL: AES should be in ALL usable\n"); return -1;
        }
        if ((all_features.usable_bits[HAL_CPU_FEATURE_VECTOR / 64u] & (1ULL << (HAL_CPU_FEATURE_VECTOR % 64u))) != 0) {
            KPRINT("FAIL: VECTOR should NOT be in ALL usable\n"); return -1;
        }

        // Expected ANY: AES, VECTOR, SHA
        if ((any_features.usable_bits[HAL_CPU_FEATURE_AES / 64u] & (1ULL << (HAL_CPU_FEATURE_AES % 64u))) == 0 ||
            (any_features.usable_bits[HAL_CPU_FEATURE_VECTOR / 64u] & (1ULL << (HAL_CPU_FEATURE_VECTOR % 64u))) == 0 ||
            (any_features.usable_bits[HAL_CPU_FEATURE_SHA / 64u] & (1ULL << (HAL_CPU_FEATURE_SHA % 64u))) == 0) {
            KPRINT("FAIL: AES, VECTOR, SHA should be in ANY usable\n"); return -1;
        }

        // Raw feature NOT usable
        if ((any_features.usable_bits[HAL_CPU_FEATURE_SCALABLE_VECTOR / 64u] & (1ULL << (HAL_CPU_FEATURE_SCALABLE_VECTOR % 64u))) != 0) {
            KPRINT("FAIL: Raw feature was improperly aggregated into usable\n"); return -1;
        }
    }

    KPRINT("PASS: Hardware Caps Freeze Conformance Tests\n");
    return 0;
}

REGISTER_BOOT_SELFTEST("hw_caps", "core", test_hw_caps_sanity, BOOT_TEST_STAGE_RUNTIME, BOOT_TEST_MANDATORY, 0, true)
