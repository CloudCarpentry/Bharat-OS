#include "../../include/tests/ktest.h"
#include "hal/hal_memops.h"
#include "console/console_core.h"
#include "../../include/kernel.h"

#define KPRINT(s) console_write_raw(s, string_length(s))

static size_t g_mock_cpu_id = 0;
static size_t mock_current_cpu(void) { return g_mock_cpu_id; }

static void* dummy_memcpy_0(void* dst, const void* src, size_t n) { (void)src; (void)n; return (void*)((uintptr_t)dst + 10); }
static void* dummy_memset_0(void* dst, int c, size_t n) { (void)c; (void)n; return (void*)((uintptr_t)dst + 10); }
static void* dummy_memmove_0(void* dst, const void* src, size_t n) { (void)src; (void)n; return (void*)((uintptr_t)dst + 10); }
static int dummy_memcmp_0(const void* lhs, const void* rhs, size_t n) { (void)lhs; (void)rhs; (void)n; return 10; }

static void* dummy_memcpy_1(void* dst, const void* src, size_t n) { (void)src; (void)n; return (void*)((uintptr_t)dst + 11); }
static void* dummy_memset_1(void* dst, int c, size_t n) { (void)c; (void)n; return (void*)((uintptr_t)dst + 11); }
static void* dummy_memmove_1(void* dst, const void* src, size_t n) { (void)src; (void)n; return (void*)((uintptr_t)dst + 11); }
static int dummy_memcmp_1(const void* lhs, const void* rhs, size_t n) { (void)lhs; (void)rhs; (void)n; return 11; }

static int test_hal_memops_dispatch(void) {
    KPRINT("Running HAL Memops Adaptive Dispatch Tests...\n");

    // We restart the memops subsystem for mock testing since we want to test all scenarios independently.
    // Ensure we are in a safe test environment to do this, hal_memops_begin clears it and unfreezes it.
    if (!hal_memops_begin(mock_current_cpu)) {
        KPRINT("FAIL: hal_memops_begin failed.\n");
        return -1;
    }

    uint8_t dst[16] = {0};
    uint8_t src[16] = {1};

    // Scenario 2: Dispatch before freeze: verify scalar fallback.
    // The backend hasn't been frozen.
    g_mock_cpu_id = 0;
    if (hal_memcpy(dst, src, 1, BH_MEMCTX_F_DEFAULT) != dst ||
        hal_memset(dst, 2, 1, BH_MEMCTX_F_DEFAULT) != dst ||
        hal_memmove(dst, src, 1, BH_MEMCTX_F_DEFAULT) != dst ||
        hal_memcmp(dst, src, 1, BH_MEMCTX_F_DEFAULT) != 0) {
        KPRINT("FAIL: Dispatch before freeze did not fall back to scalar.\n");
        return -1;
    }

    // Freeze now so we can test other scenarios (actually we need to register first, so let's re-begin)
    hal_memops_begin(mock_current_cpu);

    // Scenario 6: Missing backend function pointers: verify registration rejection.
    hal_memops_backend_t bad_backend = {
        .copy = NULL,
        .set = dummy_memset_0,
        .move = dummy_memmove_0,
        .compare = dummy_memcmp_0,
        .context_flags = BH_MEMCTX_F_DEFAULT,
        .implementation_flags = HAL_MEMOPS_IMPL_F_GPR_ONLY
    };
    if (hal_memops_register(0, &bad_backend)) {
        KPRINT("FAIL: hal_memops_register accepted incomplete backend.\n");
        return -1;
    }

    // Register valid backends
    hal_memops_backend_t backend0 = {
        .copy = dummy_memcpy_0, .set = dummy_memset_0, .move = dummy_memmove_0, .compare = dummy_memcmp_0,
        .context_flags = BH_MEMCTX_F_MAY_SLEEP,
        .implementation_flags = HAL_MEMOPS_IMPL_F_GPR_ONLY
    };

    hal_memops_backend_t backend1 = {
        .copy = dummy_memcpy_1, .set = dummy_memset_1, .move = dummy_memmove_1, .compare = dummy_memcmp_1,
        .context_flags = BH_MEMCTX_F_MAY_SLEEP,
        .implementation_flags = HAL_MEMOPS_IMPL_F_GPR_ONLY
    };

    hal_memops_register(0, &backend0);
    hal_memops_register(1, &backend1);

    if (!hal_memops_freeze()) {
        KPRINT("FAIL: hal_memops_freeze failed.\n");
        return -1;
    }

    // Scenario 8: Heterogeneous CPUs
    g_mock_cpu_id = 0;
    if (hal_memcpy(dst, src, 1, BH_MEMCTX_F_DEFAULT) != (void*)((uintptr_t)dst + 10)) {
        KPRINT("FAIL: CPU 0 did not select backend 0.\n");
        return -1;
    }
    g_mock_cpu_id = 1;
    if (hal_memcpy(dst, src, 1, BH_MEMCTX_F_DEFAULT) != (void*)((uintptr_t)dst + 11)) {
        KPRINT("FAIL: CPU 1 did not select backend 1.\n");
        return -1;
    }

    // Scenario 1: No backend registered (CPU 2)
    // Scenario 3: Current CPU has no registered backend
    g_mock_cpu_id = 2;
    if (hal_memcpy(dst, src, 1, BH_MEMCTX_F_DEFAULT) != dst ||
        hal_memset(dst, 2, 1, BH_MEMCTX_F_DEFAULT) != dst ||
        hal_memmove(dst, src, 1, BH_MEMCTX_F_DEFAULT) != dst ||
        hal_memcmp(dst, src, 1, BH_MEMCTX_F_DEFAULT) != 0) {
        KPRINT("FAIL: CPU without backend did not fall back to scalar.\n");
        return -1;
    }

    // Scenario 4: Current CPU ID is out of range
    g_mock_cpu_id = HAL_MEMOPS_MAX_CPUS + 10;
    if (hal_memcpy(dst, src, 1, BH_MEMCTX_F_DEFAULT) != dst) {
        KPRINT("FAIL: Out-of-range CPU did not fall back to scalar safely.\n");
        return -1;
    }

    // Scenario 5: Early-boot and IRQ-safe contexts
    g_mock_cpu_id = 0;
    if (hal_memcpy(dst, src, 1, BH_MEMCTX_F_EARLY_BOOT) != dst) {
        KPRINT("FAIL: EARLY_BOOT context did not fall back to scalar.\n");
        return -1;
    }
    if (hal_memcpy(dst, src, 1, BH_MEMCTX_F_IRQ_SAFE) != dst) {
        KPRINT("FAIL: IRQ_SAFE context did not fall back to scalar.\n");
        return -1;
    }

    // Scenario 9: Unsupported backend context flags
    // Let's pass a flag that is not in context_flags
    if (hal_memcpy(dst, src, 1, BH_MEMCTX_F_NO_SIMD) != dst) {
        KPRINT("FAIL: Unsupported context flags did not fall back to scalar.\n");
        return -1;
    }

    // Scenario 7: Registration after freeze
    hal_memops_backend_t backend2 = backend0;
    if (hal_memops_register(2, &backend2) != false) {
        KPRINT("FAIL: Registration after freeze succeeded.\n");
        return -1;
    }

    // Scenario 10: Verify correct memmove behavior with overlapping source and destination buffers
    uint8_t overlap_buf[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    hal_memmove_scalar(overlap_buf + 2, overlap_buf, 4);
    if (overlap_buf[2] != 1 || overlap_buf[3] != 2 || overlap_buf[4] != 3 || overlap_buf[5] != 4) {
        KPRINT("FAIL: overlapping hal_memmove_scalar failed.\n");
        return -1;
    }

    KPRINT("PASS: HAL Memops Adaptive Dispatch Tests\n");
    return 0;
}

REGISTER_BOOT_SELFTEST("hal_memops", "core", test_hal_memops_dispatch, BOOT_TEST_STAGE_RUNTIME, BOOT_TEST_MANDATORY, 0, true)
