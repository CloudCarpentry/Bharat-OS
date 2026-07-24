#include <arch/memops.h>
#include <tests/ktest.h>
#include <stdint.h>
#include <stddef.h>

#define TEST_BUF_SIZE 128

static uint8_t buf1[TEST_BUF_SIZE] __attribute__((aligned(16)));
static uint8_t buf2[TEST_BUF_SIZE] __attribute__((aligned(16)));

static int test_memcpy_aligned(void) {
    for (int i = 0; i < TEST_BUF_SIZE; i++) buf1[i] = (uint8_t)i;
    arch_memset_scalar(buf2, 0, TEST_BUF_SIZE);

    arch_memcpy(buf2, buf1, 64, ARCH_MEMOP_F_DEFAULT);
    for (int i = 0; i < 64; i++) {
        if (buf2[i] != (uint8_t)i) return -1;
    }
    for (int i = 64; i < TEST_BUF_SIZE; i++) {
        if (buf2[i] != 0) return -2;
    }
    return 0;
}

static int test_memcpy_unaligned(void) {
    for (int i = 0; i < TEST_BUF_SIZE; i++) buf1[i] = (uint8_t)i;
    arch_memset_scalar(buf2, 0, TEST_BUF_SIZE);

    // Unaligned src and dst
    arch_memcpy(buf2 + 1, buf1 + 3, 15, ARCH_MEMOP_F_DEFAULT);
    for (int i = 0; i < 15; i++) {
        if (buf2[i + 1] != (uint8_t)(i + 3)) return -1;
    }
    return 0;
}

static int test_memset_aligned(void) {
    arch_memset_scalar(buf1, 0, TEST_BUF_SIZE);
    arch_memset(buf1, 0xAA, 64, ARCH_MEMOP_F_DEFAULT);
    for (int i = 0; i < 64; i++) {
        if (buf1[i] != 0xAA) return -1;
    }
    for (int i = 64; i < TEST_BUF_SIZE; i++) {
        if (buf1[i] != 0) return -2;
    }
    return 0;
}

static int test_memset_unaligned(void) {
    arch_memset_scalar(buf1, 0, TEST_BUF_SIZE);
    arch_memset(buf1 + 1, 0xBB, 7, ARCH_MEMOP_F_DEFAULT);
    if (buf1[0] != 0) return -1;
    for (int i = 1; i < 8; i++) {
        if (buf1[i] != 0xBB) return -2;
    }
    if (buf1[8] != 0) return -3;
    return 0;
}

static int test_memmove_overlapping_forward(void) {
    for (int i = 0; i < 32; i++) buf1[i] = (uint8_t)i;
    // Copy [0..15] to [5..20] - overlap
    arch_memmove(buf1 + 5, buf1, 16, ARCH_MEMOP_F_DEFAULT);
    for (int i = 0; i < 16; i++) {
        if (buf1[i + 5] != (uint8_t)i) return -1;
    }
    return 0;
}

static int test_memmove_overlapping_backward(void) {
    for (int i = 0; i < 32; i++) buf1[i] = (uint8_t)i;
    // Copy [5..20] to [0..15] - overlap
    arch_memmove(buf1, buf1 + 5, 16, ARCH_MEMOP_F_DEFAULT);
    for (int i = 0; i < 16; i++) {
        if (buf1[i] != (uint8_t)(i + 5)) return -1;
    }
    return 0;
}

static int test_memops_zero_length(void) {
    uint8_t b = 0xFF;
    arch_memcpy(&b, buf1, 0, ARCH_MEMOP_F_DEFAULT);
    if (b != 0xFF) return -1;
    arch_memset(&b, 0, 0, ARCH_MEMOP_F_DEFAULT);
    if (b != 0xFF) return -2;
    arch_memmove(&b, buf1, 0, ARCH_MEMOP_F_DEFAULT);
    if (b != 0xFF) return -3;
    return 0;
}

static int ktest_memops_run(void) {
    if (test_memcpy_aligned() != 0) return -1;
    if (test_memcpy_unaligned() != 0) return -2;
    if (test_memset_aligned() != 0) return -3;
    if (test_memset_unaligned() != 0) return -4;
    if (test_memmove_overlapping_forward() != 0) return -5;
    if (test_memmove_overlapping_backward() != 0) return -6;
    if (test_memops_zero_length() != 0) return -7;
    return 0;
}

REGISTER_BOOT_SELFTEST("arch_memops", "arch", ktest_memops_run, BOOT_TEST_STAGE_RUNTIME, BOOT_TEST_MANDATORY, 0, false)
