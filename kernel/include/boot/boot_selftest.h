#ifndef BHARAT_BOOT_SELFTEST_H
#define BHARAT_BOOT_SELFTEST_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BOOT_TEST_STAGE_EARLY = 0,
    BOOT_TEST_STAGE_SECURITY,
    BOOT_TEST_STAGE_MEMORY,
    BOOT_TEST_STAGE_PLATFORM,
    BOOT_TEST_STAGE_RUNTIME
} boot_test_stage_t;

typedef enum {
    BOOT_TEST_MANDATORY = 0,
    BOOT_TEST_QUICK,
    BOOT_TEST_EXTENDED,
    BOOT_TEST_MANUFACTURING,
    BOOT_TEST_BENCHMARK_ONLY
} boot_test_policy_class_t;

typedef struct {
    boot_test_stage_t stage;
    boot_test_policy_class_t policy_class;
    uint64_t required_caps;
    bool fatal_on_failure;
} boot_test_meta_t;

typedef struct {
    uint32_t total_seen;
    uint32_t total_run;
    uint32_t total_passed;
    uint32_t total_failed;
    uint32_t total_skipped;
    uint32_t fatal_failures;
} boot_selftest_report_t;

bool boot_selftest_run_stage(boot_test_stage_t stage,
                             boot_selftest_report_t *report);

bool boot_selftest_should_run_class(boot_test_policy_class_t policy_class);
bool boot_selftest_is_test_allowed(const boot_test_meta_t *meta);

#endif // BHARAT_BOOT_SELFTEST_H
