#include "tests/ktest.h"
#include "kernel_safety.h"
#include "boot/boot_selftest.h"

static bool test_basic_math_invariants(void) {
    // Basic addition / subtraction
    KTEST_ASSERT((2 + 2) == 4, "Addition invariant failed");
    KTEST_ASSERT((10 - 3) == 7, "Subtraction invariant failed");

    // Shift / Mask invariants
    KTEST_ASSERT((1u << 5) == 32, "Shift invariant failed");
    KTEST_ASSERT((0xF0 & 0x0F) == 0, "Mask invariant failed");

    // Alignment macro invariants
    KTEST_ASSERT(BHARAT_IS_ALIGNED(32, 8) == true, "IS_ALIGNED aligned value failed");
    KTEST_ASSERT(BHARAT_IS_ALIGNED(33, 8) == false, "IS_ALIGNED unaligned value failed");
    KTEST_ASSERT(BHARAT_IS_ALIGNED(32, 0) == false, "IS_ALIGNED zero alignment should be false");

    // Bounds check
    KTEST_ASSERT(BHARAT_BOUNDS_CHECK(5, 10) == true, "BOUNDS_CHECK within bounds failed");
    KTEST_ASSERT(BHARAT_BOUNDS_CHECK(10, 10) == false, "BOUNDS_CHECK exact bound failed");
    KTEST_ASSERT(BHARAT_BOUNDS_CHECK(15, 10) == false, "BOUNDS_CHECK out of bounds failed");

    // Range valid
    KTEST_ASSERT(BHARAT_RANGE_VALID(5, 0, 10) == true, "RANGE_VALID within bounds failed");
    KTEST_ASSERT(BHARAT_RANGE_VALID(0, 0, 10) == true, "RANGE_VALID min bound failed");
    KTEST_ASSERT(BHARAT_RANGE_VALID(10, 0, 10) == true, "RANGE_VALID max bound failed");
    KTEST_ASSERT(BHARAT_RANGE_VALID(15, 0, 10) == false, "RANGE_VALID out of bounds failed");

    return true;
}

static ktest_case_t basic_sanity_tests[] = {
    {"Basic Math & Helper Invariants", test_basic_math_invariants}
};

static int boot_test_basic_sanity(void) {
    ktest_run_suite("Basic Sanity Boot Tests", basic_sanity_tests, 1);
    if (test_basic_math_invariants()) {
        return 0; // success
    }
    return -1;
}

REGISTER_BOOT_SELFTEST("basic_math_sanity", "core", boot_test_basic_sanity, BOOT_TEST_STAGE_EARLY, BOOT_TEST_MANDATORY, 0, true)
