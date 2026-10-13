#include "../../include/tests/ktest.h"
#include "hal/hal_memops.h"
#include "console/console_core.h"
#include "../../include/kernel.h"

#define KPRINT(s) console_write_raw(s, string_length(s))

static int test_hal_memops_dispatch(void) {
    KPRINT("Running HAL Memops Adaptive Dispatch Tests...\n");

    // In a running kernel, memops must be frozen.
    if (!hal_memops_is_frozen()) {
        KPRINT("FAIL: hal_memops_is_frozen returned false, expected true.\n");
        return -1;
    }

    // Re-initialization or mutation after freeze must be rejected.
    if (hal_memops_begin(NULL) != false) {
        KPRINT("FAIL: hal_memops_begin accepted after freeze.\n");
        return -1;
    }

    if (hal_memops_freeze() != false) {
        KPRINT("FAIL: hal_memops_freeze accepted double freeze.\n");
        return -1;
    }

    hal_memops_backend_t backend = {0};
    if (hal_memops_register(0, &backend) != false) {
        KPRINT("FAIL: hal_memops_register accepted after freeze.\n");
        return -1;
    }

    // Functional memory operations testing
    uint8_t src[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    uint8_t dst[16] = {0};

    // Test hal_memcpy
    void *res = hal_memcpy(dst, src, sizeof(src), BH_MEMCTX_F_DEFAULT);
    if (res != dst) {
        KPRINT("FAIL: hal_memcpy return value mismatch.\n");
        return -1;
    }
    for (size_t i = 0; i < sizeof(src); ++i) {
        if (dst[i] != src[i]) {
            KPRINT("FAIL: hal_memcpy byte mismatch.\n");
            return -1;
        }
    }

    // Test hal_memcmp
    if (hal_memcmp(dst, src, sizeof(src), BH_MEMCTX_F_DEFAULT) != 0) {
        KPRINT("FAIL: hal_memcmp reported difference for identical buffers.\n");
        return -1;
    }
    dst[8] = 0xFF;
    if (hal_memcmp(dst, src, sizeof(src), BH_MEMCTX_F_DEFAULT) == 0) {
        KPRINT("FAIL: hal_memcmp failed to detect difference.\n");
        return -1;
    }

    // Test hal_memset
    hal_memset(dst, 0xAA, sizeof(dst), BH_MEMCTX_F_DEFAULT);
    for (size_t i = 0; i < sizeof(dst); ++i) {
        if (dst[i] != 0xAA) {
            KPRINT("FAIL: hal_memset byte mismatch.\n");
            return -1;
        }
    }

    // Test hal_memmove overlapping (forward and backward)
    uint8_t overlap_buf[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    hal_memmove(overlap_buf + 2, overlap_buf, 4, BH_MEMCTX_F_DEFAULT);
    if (overlap_buf[2] != 1 || overlap_buf[3] != 2 || overlap_buf[4] != 3 || overlap_buf[5] != 4) {
        KPRINT("FAIL: overlapping hal_memmove forward failed.\n");
        return -1;
    }

    uint8_t overlap_buf2[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    hal_memmove(overlap_buf2, overlap_buf2 + 2, 4, BH_MEMCTX_F_DEFAULT);
    if (overlap_buf2[0] != 3 || overlap_buf2[1] != 4 || overlap_buf2[2] != 5 || overlap_buf2[3] != 6) {
        KPRINT("FAIL: overlapping hal_memmove backward failed.\n");
        return -1;
    }

    // Test IRQ-safe and early-boot contexts
    uint8_t irq_buf[4] = {0};
    hal_memcpy(irq_buf, src, sizeof(irq_buf), BH_MEMCTX_F_IRQ_SAFE);
    if (irq_buf[0] != 0 || irq_buf[3] != 3) {
        KPRINT("FAIL: hal_memcpy IRQ-safe context failed.\n");
        return -1;
    }

    hal_memcpy(irq_buf, src, sizeof(irq_buf), BH_MEMCTX_F_EARLY_BOOT);
    if (irq_buf[0] != 0 || irq_buf[3] != 3) {
        KPRINT("FAIL: hal_memcpy EARLY_BOOT context failed.\n");
        return -1;
    }

    KPRINT("PASS: HAL Memops Adaptive Dispatch Tests\n");
    return 0;
}

REGISTER_BOOT_SELFTEST("hal_memops", "core", test_hal_memops_dispatch, BOOT_TEST_STAGE_RUNTIME, BOOT_TEST_MANDATORY, 0, true)

