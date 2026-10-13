#include "benchmark/benchmark.h"
#include "arch/capabilities.h"
#include "time/ktime.h"
#include "console/console_core.h"

uint64_t benchmark_get_time_ns(void) {
    return bh_ktime_now();
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

static uint32_t benchmark_pack_hw_caps(void) {
    uint32_t caps = 0;

    if (arch_has_feature_avx2()) {
        caps |= (1u << 0);
    }
    if (arch_has_feature_fma()) {
        caps |= (1u << 1);
    }
    if (arch_has_feature_aes()) {
        caps |= (1u << 2);
    }
    if (arch_has_feature_vector()) {
        caps |= (1u << 3);
    }
    if (arch_has_feature_crypto()) {
        caps |= (1u << 4);
    }

    return caps;
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
    out_result->hw_caps = benchmark_pack_hw_caps();
}

void benchmark_print(const benchmark_result_t* result, const char* name) {
    if (!result) return;

    console_log(CONSOLE_LEVEL_INFO, "--- Benchmark: %s ---\n", name ? name : "Unknown");
    console_log(CONSOLE_LEVEL_INFO, "Level     : %d\n", result->level);
    console_log(CONSOLE_LEVEL_INFO, "Latency   : %llu ns/op\n", (unsigned long long)result->latency_ns);
    console_log(CONSOLE_LEVEL_INFO, "Cycles    : %llu cycles/op\n", (unsigned long long)result->cycles);
    console_log(CONSOLE_LEVEL_INFO, "Throughput: %llu ops/sec\n", (unsigned long long)result->throughput);
    console_log(CONSOLE_LEVEL_INFO, "Mem Overhead: %llu bytes\n", (unsigned long long)result->memory_overhead);
    console_log(CONSOLE_LEVEL_INFO, "HW Caps   : 0x%08x", result->hw_caps);
    if (result->hw_caps) {
        console_log(CONSOLE_LEVEL_INFO, " (");
        int first = 1;
        if (result->hw_caps & (1u << 0)) { console_log(CONSOLE_LEVEL_INFO, "%sAVX2", first ? "" : ", "); first = 0; }
        if (result->hw_caps & (1u << 1)) { console_log(CONSOLE_LEVEL_INFO, "%sFMA", first ? "" : ", "); first = 0; }
        if (result->hw_caps & (1u << 2)) { console_log(CONSOLE_LEVEL_INFO, "%sAES", first ? "" : ", "); first = 0; }
        if (result->hw_caps & (1u << 3)) { console_log(CONSOLE_LEVEL_INFO, "%sVector", first ? "" : ", "); first = 0; }
        if (result->hw_caps & (1u << 4)) { console_log(CONSOLE_LEVEL_INFO, "%sCrypto", first ? "" : ", "); first = 0; }
        console_log(CONSOLE_LEVEL_INFO, ")");
    }
    console_log(CONSOLE_LEVEL_INFO, "\n");
    console_log(CONSOLE_LEVEL_INFO, "---------------------------\n");
}
