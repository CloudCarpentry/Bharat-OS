#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "benchmark/benchmark.h"
#include "../../kernel/include/arch/memops.h"

#define ARM64_PAGE_MASK (~0xFFFULL)

// Arch-private raw descriptor
typedef uint64_t pte_raw_t;

typedef struct {
    pte_raw_t entries[512];
} pt_t;

// Simulated arch_memset for baseline testing (as it would be provided by the actual architecture)
void *mock_arch_memset(void *dst, int c, size_t n, uint32_t flags) {
    (void)flags;
    return memset(dst, c, n);
}

void benchmark_pt_zero_manual() {
    const int ITERATIONS = 100000;
    pt_t table;
    pt_t* ptr = &table;

    benchmark_ctx_t ctx;
    benchmark_start(&ctx, "arm64 page-table zero init (manual loop)", BENCHMARK_LEVEL_0_REF, ITERATIONS);

    for (int iter = 0; iter < ITERATIONS; iter++) {
        for(int i=0; i<512; i++) ptr->entries[i] = 0;

        // Prevent compiler from optimizing away the loop completely.
        asm volatile("" : : "r"(ptr->entries[0]) : "memory");
    }

    benchmark_stop(&ctx);

    benchmark_result_t result;
    benchmark_record(&ctx, &result);
    benchmark_print(&result, ctx.name);
}

void benchmark_pt_zero_memset() {
    const int ITERATIONS = 100000;
    pt_t table;
    pt_t* ptr = &table;

    benchmark_ctx_t ctx;
    benchmark_start(&ctx, "arm64 page-table zero init (arch_memset)", BENCHMARK_LEVEL_0_REF, ITERATIONS);

    for (int iter = 0; iter < ITERATIONS; iter++) {
        mock_arch_memset(ptr, 0, sizeof(*ptr), ARCH_MEMOP_F_DEFAULT);

        // Prevent compiler from optimizing away the memset completely.
        asm volatile("" : : "r"(ptr->entries[0]) : "memory");
    }

    benchmark_stop(&ctx);

    benchmark_result_t result;
    benchmark_record(&ctx, &result);
    benchmark_print(&result, ctx.name);
}

int main() {
    printf("Running ARM64 PT Zero Benchmarks...\n");

    // Warmup
    pt_t table;
    for(int i=0; i<1000; i++) {
        for(int j=0; j<512; j++) table.entries[j] = 0;
        mock_arch_memset(&table, 0, sizeof(table), ARCH_MEMOP_F_DEFAULT);
    }

    benchmark_pt_zero_manual();
    benchmark_pt_zero_memset();
    return 0;
}
