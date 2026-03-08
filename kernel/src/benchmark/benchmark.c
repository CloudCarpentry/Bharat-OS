#include "benchmark/benchmark.h"
#include <stdio.h>

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#include <time.h>
#endif

// Fallback implementations for host-level testing
uint64_t benchmark_get_time_ns(void) {
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#else
    // Dummy fallback
    return 0;
#endif
}

uint64_t benchmark_get_cycles(void) {
#if defined(__x86_64__) || defined(_M_X64)
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r" (val));
    return val;
#elif defined(__riscv) && __riscv_xlen == 64
    uint64_t cycles;
    __asm__ __volatile__("csrr %0, cycle" : "=r"(cycles));
    return cycles;
#else
    // Dummy fallback
    return 0;
#endif
}

uint64_t benchmark_get_memory_usage(void) {
    // A real implementation would query `pmm_get_used_memory()` or similar inside the kernel
    // For now, this is a placeholder
    return 0;
}

void benchmark_start(benchmark_ctx_t* ctx, const char* name, benchmark_level_t level, uint64_t iterations) {
    if (!ctx) return;
    ctx->name = name;
    ctx->level = level;
    ctx->iterations = iterations;
    ctx->start_memory = benchmark_get_memory_usage();
    ctx->start_time_ns = benchmark_get_time_ns();
    ctx->start_cycles = benchmark_get_cycles();
}

void benchmark_stop(benchmark_ctx_t* ctx) {
    if (!ctx) return;
    ctx->end_cycles = benchmark_get_cycles();
    ctx->end_time_ns = benchmark_get_time_ns();
    ctx->end_memory = benchmark_get_memory_usage();
}

void benchmark_record(const benchmark_ctx_t* ctx, benchmark_result_t* out_result) {
    if (!ctx || !out_result) return;

    uint64_t delta_ns = ctx->end_time_ns - ctx->start_time_ns;
    uint64_t delta_cycles = ctx->end_cycles - ctx->start_cycles;

    out_result->latency_ns = (ctx->iterations > 0) ? (delta_ns / ctx->iterations) : 0;
    out_result->cycles = (ctx->iterations > 0) ? (delta_cycles / ctx->iterations) : 0;
    out_result->throughput = (delta_ns > 0) ? ((ctx->iterations * 1000000000ULL) / delta_ns) : 0;

    out_result->memory_overhead = (ctx->end_memory > ctx->start_memory) ? (ctx->end_memory - ctx->start_memory) : 0;

    out_result->level = ctx->level;
    out_result->hw_caps = 0; // TODO: Fetch from `g_arch_caps` later
}

void benchmark_print(const benchmark_result_t* result, const char* name) {
    if (!result) return;

    printf("--- Benchmark: %s ---\n", name ? name : "Unknown");
    printf("Level     : %d\n", result->level);
    printf("Latency   : %llu ns/op\n", (unsigned long long)result->latency_ns);
    printf("Cycles    : %llu cycles/op\n", (unsigned long long)result->cycles);
    printf("Throughput: %llu ops/sec\n", (unsigned long long)result->throughput);
    printf("Mem Overhead: %llu bytes\n", (unsigned long long)result->memory_overhead);
    printf("---------------------------\n");
}
