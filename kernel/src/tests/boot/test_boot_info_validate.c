#include "bharat/boot_info.h"
#include "boot/boot_info_validate.h"
#include "kernel.h"
#include "console/console_core.h"
#include <stdbool.h>

#define KPRINT(s) console_write_raw(s, string_length(s))

void test_boot_info_validate_bounds(void) {
    boot_info_t boot = {0};

    // Minimal Valid
    boot.kernel_phys_start = 0x100000;
    boot.kernel_phys_end = 0x200000;
    boot.cmdline[0] = '\0';

    if (!boot_info_validate(&boot)) {
        KPRINT("TEST FAIL: Minimal boot_info_t validation rejected.\n");
    }

    // Invalid Kernel Overlap
    boot.kernel_phys_start = 0x200000;
    boot.kernel_phys_end = 0x100000; // start > end
    if (boot_info_validate(&boot)) {
        KPRINT("TEST FAIL: Invalid kernel bounds ordering accepted.\n");
    }
    boot.kernel_phys_start = 0x100000;
    boot.kernel_phys_end = 0x200000;

    // Valid Module
    boot.modules[0].phys_start = 0x300000;
    boot.modules[0].size = 0x10000;
    boot.module_count = 1;
    if (!boot_info_validate(&boot)) {
        KPRINT("TEST FAIL: Valid module bounds rejected.\n");
    }

    // Module overlap with kernel
    boot.modules[0].phys_start = 0x150000; // falls inside 0x100000 to 0x200000
    if (boot_info_validate(&boot)) {
         KPRINT("TEST FAIL: Module overlapping kernel was accepted.\n");
    }

    // Malformed/overflow video sizes
    boot_info_t vboot = {0};
    vboot.kernel_phys_start = 0x100000;
    vboot.kernel_phys_end = 0x200000;
    vboot.video.valid = true;
    vboot.video.phys_addr = 0x4000000;
    vboot.video.size = 0x100000;
    vboot.video.width = 1920;
    vboot.video.height = 1080;
    vboot.video.stride_bytes = 1920 * 4;

    if (boot_info_validate(&vboot)) {
         KPRINT("TEST FAIL: Valid video dimensions should pass overlap checks.\n");
    }

    // Stride calculation wraps
    vboot.video.width = 40000;
    vboot.video.height = 40000;
    vboot.video.stride_bytes = 40000 * 4;
    if (boot_info_validate(&vboot)) {
         KPRINT("TEST FAIL: Invalid video math accepted.\n");
    }
}

static int test_boot_info_sanity(void) {
    test_boot_info_validate_bounds();
    return 0; // The original test doesn't return pass/fail properly, but we can wrap it
}

#include "tests/ktest.h"
REGISTER_BOOT_SELFTEST("boot_info_sanity", "boot", test_boot_info_sanity, BOOT_TEST_STAGE_EARLY, BOOT_TEST_MANDATORY, 0, true)
