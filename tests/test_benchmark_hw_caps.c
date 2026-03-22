#include "benchmark/benchmark.h"
#include "arch/capabilities.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int g_mock_caps = 0;

bool arch_cpu_has(int feature) {
    return (g_mock_caps & (1u << feature)) != 0;
}

int main(void) {
    // Reset global caps to 0 initially
    g_mock_caps = 0;

    // Force a known value
    g_mock_caps |= (1u << 0); // has_avx2
    g_mock_caps |= (1u << 1); // has_fma
    g_mock_caps |= (1u << 3); // has_vector

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
