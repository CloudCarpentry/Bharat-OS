#include "bharat/hw_caps_registry.h"
#include "kernel_safety.h"
#include "tests/ktest.h"
#include "boot/boot_selftest.h"

static bharat_hw_caps_t g_registry_caps;
static bool g_registry_initialized = false;

int hw_caps_registry_init(const bharat_hw_caps_t *caps) {
    if (!caps) return -1;

    // Immutable registration: cannot be updated once set
    if (g_registry_initialized) return -1;

    // Perform copy
    g_registry_caps = *caps;
    g_registry_initialized = true;

    return 0;
}

int hw_caps_registry_get_global(bharat_hw_caps_t *out_caps) {
    if (!out_caps) return -1;
    if (!g_registry_initialized) return -1;

    *out_caps = g_registry_caps;
    return 0;
}

static int hw_caps_registry_selftest_internal(void) {
    bharat_hw_caps_t out_caps;
    int ret;

    if (!g_registry_initialized) {
        // If not initialized by platform yet, do a fake init to test
        bharat_hw_caps_t fake_caps = {0};
        fake_caps.cpu.vector_simd = HW_CAP_STATE_PRESENT;
        hw_caps_registry_init(&fake_caps);
    }

    // Test retrieval
    ret = hw_caps_registry_get_global(&out_caps);
    KTEST_ASSERT(ret == 0, "hw_caps_registry_get_global failed");

    // Test immutability
    bharat_hw_caps_t override_caps = {0};
    override_caps.cpu.vector_simd = HW_CAP_STATE_ABSENT;
    ret = hw_caps_registry_init(&override_caps);
    KTEST_ASSERT(ret != 0, "hw_caps_registry_init allowed overwrite");

    return 0; // Return 0 on success for new ktest format
}

// Hook into the kernel selftest framework
REGISTER_BOOT_SELFTEST("hw_caps_registry",
                       "core",
                       hw_caps_registry_selftest_internal,
                       BOOT_TEST_STAGE_EARLY,
                       BOOT_TEST_MANDATORY,
                       0,
                       true);
