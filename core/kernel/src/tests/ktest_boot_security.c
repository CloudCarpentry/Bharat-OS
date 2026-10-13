#include "kernel.h"
#include "boot/boot_security.h"
#include "console/console_core.h"
#include "tests/ktest.h"
#include "lib/base/string.h"

static int test_boot_security_strict_verified(void) {
    boot_info_t bi;
    memset(&bi, 0, sizeof(bi));
    bi.selected_mode = BOOT_MODE_NORMAL;
    bi.security_state = BOOT_SECURITY_SECURE_VERIFIED;
    bi.security_info.secure_boot_verified = true;

    boot_security_decision_t decision;
    int res = boot_security_evaluate(&bi, &decision);
    if (res != 0 || decision != BOOT_SEC_DECISION_ALLOW) return -1;
    if (!boot_security_allows_mode(&bi, BOOT_MODE_NORMAL)) return -1;

    return 0;
}

static int test_boot_security_strict_unverified(void) {
    boot_info_t bi;
    memset(&bi, 0, sizeof(bi));
    bi.selected_mode = BOOT_MODE_NORMAL;
    bi.security_state = BOOT_SECURITY_SECURE_UNVERIFIED;
    bi.security_info.secure_boot_verified = false;

    boot_security_decision_t decision;
    int res = boot_security_evaluate(&bi, &decision);
    if (res != 0 || decision != BOOT_SEC_DECISION_DENY) return -1;
    if (boot_security_allows_mode(&bi, BOOT_MODE_NORMAL)) return -1;

    return 0;
}

static int test_boot_security_strict_unsupported(void) {
    boot_info_t bi;
    memset(&bi, 0, sizeof(bi));
    bi.selected_mode = BOOT_MODE_NORMAL;
    bi.security_state = BOOT_SECURITY_UNKNOWN;
    bi.security_info.secure_boot_verified = false;

    boot_security_decision_t decision;
    int res = boot_security_evaluate(&bi, &decision);
    if (res != 0 || decision != BOOT_SEC_DECISION_DENY) return -1;
    if (boot_security_allows_mode(&bi, BOOT_MODE_NORMAL)) return -1;

    return 0;
}

static int test_boot_security_permissive_unverified(void) {
    boot_info_t bi;
    memset(&bi, 0, sizeof(bi));
    bi.selected_mode = BOOT_MODE_LEGACY_BRINGUP;
    bi.security_state = BOOT_SECURITY_SECURE_UNVERIFIED;
    bi.security_info.secure_boot_verified = false;

    boot_security_decision_t decision;
    int res = boot_security_evaluate(&bi, &decision);
    if (res != 0 || decision != BOOT_SEC_DECISION_WARN_AND_ALLOW) return -1;
    if (!boot_security_allows_mode(&bi, BOOT_MODE_LEGACY_BRINGUP)) return -1;

    return 0;
}

static int test_boot_security_recovery_unverified(void) {
    boot_info_t bi;
    memset(&bi, 0, sizeof(bi));
    bi.selected_mode = BOOT_MODE_RECOVERY;
    bi.security_state = BOOT_SECURITY_SECURE_UNVERIFIED;
    bi.security_info.secure_boot_verified = false;

    boot_security_decision_t decision;
    int res = boot_security_evaluate(&bi, &decision);
    if (res != 0 || decision != BOOT_SEC_DECISION_WARN_AND_ALLOW) return -1;
    if (!boot_security_allows_mode(&bi, BOOT_MODE_RECOVERY)) return -1;

    return 0;
}

static int test_boot_security_debug_restricted(void) {
    boot_info_t bi;
    memset(&bi, 0, sizeof(bi));
    bi.selected_mode = BOOT_MODE_DEBUG;
    bi.security_state = BOOT_SECURITY_SECURE_UNVERIFIED;
    bi.security_info.secure_boot_verified = false;

    boot_security_decision_t decision;
    int res = boot_security_evaluate(&bi, &decision);
    // debug shouldn't automatically bypass strict policy
    if (res != 0 || decision != BOOT_SEC_DECISION_DENY) return -1;
    if (boot_security_allows_mode(&bi, BOOT_MODE_DEBUG)) return -1;

    return 0;
}

static int test_boot_security_run(void) {
    if (test_boot_security_strict_verified() != 0) return -1;
    if (test_boot_security_strict_unverified() != 0) return -1;
    if (test_boot_security_strict_unsupported() != 0) return -1;
    if (test_boot_security_permissive_unverified() != 0) return -1;
    if (test_boot_security_recovery_unverified() != 0) return -1;
    if (test_boot_security_debug_restricted() != 0) return -1;

    console_write_raw("ktest_boot_security_run PASS\n", 29);
    return 0;
}

REGISTER_BOOT_SELFTEST("boot_security_contract", "security", test_boot_security_run, BOOT_TEST_STAGE_RUNTIME, BOOT_TEST_MANDATORY, 0, false);
