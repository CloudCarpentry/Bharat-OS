#ifndef BHARAT_OS_BENCH_H
#define BHARAT_OS_BENCH_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Benchmark maturity levels
 * B0: Experimental, report only
 * B1: Stable, tracked
 * B2: Regression-gated in CI
 * B3: Release-signoff benchmark
 */
typedef enum {
    BENCH_LEVEL_B0,
    BENCH_LEVEL_B1,
    BENCH_LEVEL_B2,
    BENCH_LEVEL_B3
} bench_level_t;

/**
 * Core benchmark case definition
 */
typedef struct {
    const char *name;
    const char *group;
    bench_level_t level;
    uint64_t default_iterations;
    uint64_t warmup_iterations;

    // Lifecycle hooks
    int (*setup)(void **ctx);
    int (*run)(void *ctx, uint64_t iterations);
    void (*teardown)(void *ctx);
} bench_case_t;

// Macro to define and register a benchmark case
#define REGISTER_BENCHMARK(name_val, group_val, level_val, iters, warmup, setup_fn, run_fn, teardown_fn) \
    static const bench_case_t __bench_##name_val \
    __attribute__((used, section("benchmarks"))) = { \
        .name = #name_val, \
        .group = group_val, \
        .level = level_val, \
        .default_iterations = iters, \
        .warmup_iterations = warmup, \
        .setup = setup_fn, \
        .run = run_fn, \
        .teardown = teardown_fn \
    }

#ifdef __cplusplus
}
#endif

#endif // BHARAT_OS_BENCH_H
