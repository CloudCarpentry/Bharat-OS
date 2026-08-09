#include "kernel/primitive.h"
#include "kernel/primitive_caps.h"
#include "hal/hal_hw_caps.h"
#include "tests/ktest.h"
#include "boot/boot_selftest.h"

// Note: Testing immutable finalized state requires mocking, but since we use
// the real registry, we can test that calling bh_kernel_primitive_registry_init
// AGAIN returns K_ERR_BAD_STATE (already finalized).
// Because the boot process already initialized it, we can verify it rejects a second init.
static int kprim_caps_selftest_reinit(void) {
    hal_hw_caps_t caps = {0};
    kstatus_t status = bh_kernel_primitive_registry_init(&caps);
    KTEST_ASSERT(status == K_ERR_BAD_STATE, "kprim registry should reject duplicate initialization");
    return 0;
}

static int kprim_caps_selftest_queries(void) {
    // We assume the system initialized correctly (since we're running).
    // Test capability queries for boundary capabilities.

    // A valid capability ID should either be true or false without crashing
    bool has_timer = bh_kprim_has(BH_KPRIM_CAP_HIGH_RES_TIMER);
    bh_primitive_support_level_t timer_support = bh_kprim_get_support_level(BH_KPRIM_CAP_HIGH_RES_TIMER);

    // If it has the timer, the support level should be > UNSUPPORTED
    if (has_timer) {
        KTEST_ASSERT(timer_support > BH_PRIMITIVE_UNSUPPORTED, "timer present but unsupported");
    } else {
        KTEST_ASSERT(timer_support == BH_PRIMITIVE_UNSUPPORTED, "timer absent but supported");
    }

    // Invalid capability ID should return false/unsupported
    bool invalid_cap = bh_kprim_has((bh_kprim_capability_t)9999);
    KTEST_ASSERT(invalid_cap == false, "invalid capability ID should return false");

    bh_primitive_support_level_t invalid_support = bh_kprim_get_support_level((bh_kprim_capability_t)9999);
    KTEST_ASSERT(invalid_support == BH_PRIMITIVE_UNSUPPORTED, "invalid capability ID should be unsupported");

    bool invalid_cap_any = bh_kprim_has_any((bh_kprim_capability_t)9999);
    KTEST_ASSERT(invalid_cap_any == false, "invalid capability ID any should return false");

    bool invalid_cap_local = bh_kprim_has_local((bh_kprim_capability_t)9999);
    KTEST_ASSERT(invalid_cap_local == false, "invalid capability ID local should return false");

    return 0;
}

REGISTER_BOOT_SELFTEST("kprim_caps_reinit",
                       "core",
                       kprim_caps_selftest_reinit,
                       BOOT_TEST_STAGE_RUNTIME,
                       BOOT_TEST_MANDATORY,
                       0,
                       true);

REGISTER_BOOT_SELFTEST("kprim_caps_queries",
                       "core",
                       kprim_caps_selftest_queries,
                       BOOT_TEST_STAGE_RUNTIME,
                       BOOT_TEST_MANDATORY,
                       0,
                       true);
