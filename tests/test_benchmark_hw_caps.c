#include "benchmark/benchmark.h"
#include "arch/capabilities.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void) {
    // Reset global caps to 0 initially
    memset(&g_arch_caps, 0, sizeof(g_arch_caps));

    // Force a known value
    g_arch_caps.has_avx2 = 1;
    g_arch_caps.has_fma = 1;
    g_arch_caps.has_aes = 0;
    g_arch_caps.has_vector = 1;
    g_arch_caps.has_crypto = 0;

    // Expected packed value: 0b01011 = 11

    benchmark_ctx_t ctx;
    benchmark_start(&ctx, "test_hw_caps", BENCHMARK_LEVEL_2_ISA, 1);
    benchmark_stop(&ctx);

    benchmark_result_t result;
    benchmark_record(&ctx, &result);

    printf("Packed HW caps: 0x%x\n", result.hw_caps);
    assert(result.hw_caps == 11);

    // Call benchmark_print to visually verify
    benchmark_print(&result, "Test Benchmark Print");

    printf("test_benchmark_hw_caps passed.\n");

    return 0;
}
