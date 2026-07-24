#include "boot/boot_selftest.h"
#include "boot/boot_mode.h"
#include "tests/ktest.h"
#include "arch/arch_caps.h"
#include "console/console_core.h"
#include "kernel.h"
#include <stddef.h>

#define KPRINT(s) console_write_raw(s, string_length(s))

extern const kernel_test_t __kern_tests_start[];
extern const kernel_test_t __kern_tests_end[];

static void print_uint32(uint32_t val) {
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    if (val == 0) {
        KPRINT("0");
        return;
    }
    while (val > 0 && i >= 0) {
        buf[i--] = '0' + (val % 10);
        val /= 10;
    }
    KPRINT(&buf[i + 1]);
}

bool boot_selftest_should_run_class(boot_test_policy_class_t policy_class) {
    bharat_boot_mode_t mode = bharat_boot_mode_select();

    switch (mode) {
        case BHARAT_BOOT_MODE_NORMAL:
            return (policy_class == BOOT_TEST_MANDATORY);

        case BHARAT_BOOT_MODE_DIAGNOSTIC:
            return (policy_class == BOOT_TEST_MANDATORY ||
                    policy_class == BOOT_TEST_QUICK);

        case BHARAT_BOOT_MODE_RECOVERY:
            return (policy_class == BOOT_TEST_MANDATORY);

        case BHARAT_BOOT_MODE_MANUFACTURING:
            return (policy_class == BOOT_TEST_MANDATORY ||
                    policy_class == BOOT_TEST_QUICK ||
                    policy_class == BOOT_TEST_MANUFACTURING);

        case BHARAT_BOOT_MODE_BENCHMARK:
            return (policy_class == BOOT_TEST_MANDATORY ||
                    policy_class == BOOT_TEST_BENCHMARK_ONLY);

        case BHARAT_BOOT_MODE_LEGACY_BRINGUP:
            return (policy_class == BOOT_TEST_MANDATORY ||
                    policy_class == BOOT_TEST_QUICK);

        default:
            return (policy_class == BOOT_TEST_MANDATORY);
    }
}

bool boot_selftest_is_test_allowed(const boot_test_meta_t *meta) {
    if (!meta) return false;

    arch_caps_t arch_caps = arch_get_caps();

    // Check architectural capabilities against meta->required_caps
    // The required caps is just a uint64_t from ARCH_CAP_* defines.
    // Ensure all required caps are present.
    if ((meta->required_caps & ARCH_CAP_MMU_FULL) && !arch_caps_test(arch_caps, ARCH_CAP_MMU_FULL)) return false;
    if ((meta->required_caps & ARCH_CAP_MMU_LITE) && !arch_caps_test(arch_caps, ARCH_CAP_MMU_LITE)) return false;
    if ((meta->required_caps & ARCH_CAP_MPU_ONLY) && !arch_caps_test(arch_caps, ARCH_CAP_MPU_ONLY)) return false;
    if ((meta->required_caps & ARCH_CAP_SMP) && !arch_caps_test(arch_caps, ARCH_CAP_SMP)) return false;

    return true;
}

static const char* stage_to_str(boot_test_stage_t stage) {
    switch(stage) {
        case BOOT_TEST_STAGE_EARLY: return "early";
        case BOOT_TEST_STAGE_MEMORY: return "memory";
        case BOOT_TEST_STAGE_IPC: return "ipc";
        case BOOT_TEST_STAGE_SECURITY: return "security";
        case BOOT_TEST_STAGE_PLATFORM: return "platform";
        case BOOT_TEST_STAGE_RUNTIME: return "runtime";
        default: return "unknown";
    }
}

bool boot_selftest_run_stage(boot_test_stage_t stage,
                             boot_selftest_report_t *report) {
    if (report) {
        report->total_seen = 0;
        report->total_run = 0;
        report->total_passed = 0;
        report->total_failed = 0;
        report->total_skipped = 0;
        report->fatal_failures = 0;
    }

    const kernel_test_t *t = __kern_tests_start;

    // Iterate to see if there are any tests for this stage at all before printing stage header
    bool has_tests_for_stage = false;
    const kernel_test_t *iter = t;
    while (iter < __kern_tests_end) {
        if (iter->has_boot_meta && iter->boot_meta.stage == stage) {
            has_tests_for_stage = true;
            break;
        }
        iter++;
    }

    if (has_tests_for_stage) {
        KPRINT("  [SELFTEST] Stage=");
        KPRINT(stage_to_str(stage));
        KPRINT("\n");
    } else {
        return true;
    }

    while (t < __kern_tests_end) {
        if (!t->has_boot_meta) {
            t++;
            continue;
        }

        const boot_test_meta_t *meta = &t->boot_meta;

        // Skip if wrong stage
        if (meta->stage != stage) {
            t++;
            continue;
        }

        if (report) report->total_seen++;

        // Check capability
        if (!boot_selftest_is_test_allowed(meta)) {
            KPRINT("  [SELFTEST] Skipping ");
            KPRINT(t->name);
            KPRINT(" (capability unavailable)\n");
            if (report) report->total_skipped++;
            t++;
            continue;
        }

        // Check if policy allows running
        if (!boot_selftest_should_run_class(meta->policy_class)) {
            t++;
            continue;
        }

        KPRINT("  [SELFTEST] Running ");
        KPRINT(t->name);
        KPRINT("\n");

        if (report) report->total_run++;

        int result = t->run();

        if (result == 0) {
            if (report) report->total_passed++;
        } else {
            if (report) report->total_failed++;

            if (meta->fatal_on_failure) {
                if (report) report->fatal_failures++;
                KPRINT("  [SELFTEST] Fatal boot self-test failure: ");
                KPRINT(t->name);
                KPRINT("\n");
                kernel_panic("fatal boot self-test failure");
            }
        }

        t++;
    }

    if (has_tests_for_stage) {
        KPRINT("  [SELFTEST] Stage ");
        KPRINT(stage_to_str(stage));
        KPRINT(" summary: run=");
        print_uint32(report ? report->total_run : 0);
        KPRINT(" pass=");
        print_uint32(report ? report->total_passed : 0);
        KPRINT(" fail=");
        print_uint32(report ? report->total_failed : 0);
        KPRINT(" skip=");
        print_uint32(report ? report->total_skipped : 0);
        KPRINT("\n");
    }

    if (report && report->fatal_failures > 0) return false;

    return true;
}
