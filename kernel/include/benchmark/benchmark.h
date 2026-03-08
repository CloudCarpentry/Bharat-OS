#ifndef BHARAT_BENCHMARK_H
#define BHARAT_BENCHMARK_H

#include <stdint.h>
#include <stddef.h>

/*
 * Bharat-OS Benchmark Framework
 * Used for subsystem-specific proof of capability and performance.
 */

// Implementation levels for benchmarking capability matrix
typedef enum {
    BENCHMARK_LEVEL_0_REF = 0,    // Portable reference implementation
    BENCHMARK_LEVEL_1_SMP = 1,    // SMP/concurrency-aware kernel implementation
    BENCHMARK_LEVEL_2_ISA = 2,    // ISA-tuned version
    BENCHMARK_LEVEL_3_HW  = 3     // Accelerator/offload version
} benchmark_level_t;

// Result metrics
typedef struct {
    uint64_t cycles;          // Total CPU cycles
    uint64_t latency_ns;      // Latency in nanoseconds
    uint64_t tail_latency_ns; // p95 or p99 tail latency (optional)
    uint64_t throughput;      // Operations per second
    uint64_t memory_overhead; // Memory overhead in bytes
    benchmark_level_t level;  // Implementation level chosen
    uint32_t hw_caps;         // Hardware capabilities detected/used
} benchmark_result_t;

// Context structure for a running benchmark
typedef struct {
    const char* name;
    benchmark_level_t level;
    uint64_t start_time_ns;
    uint64_t end_time_ns;
    uint64_t start_cycles;
    uint64_t end_cycles;
    uint64_t iterations;
    uint64_t start_memory;
    uint64_t end_memory;
} benchmark_ctx_t;

// Benchmark Lifecycle
void benchmark_start(benchmark_ctx_t* ctx, const char* name, benchmark_level_t level, uint64_t iterations);
void benchmark_stop(benchmark_ctx_t* ctx);
void benchmark_record(const benchmark_ctx_t* ctx, benchmark_result_t* out_result);
void benchmark_print(const benchmark_result_t* result, const char* name);

// Timer abstractions (architecture-specific or host-level POSIX fallback)
uint64_t benchmark_get_time_ns(void);
uint64_t benchmark_get_cycles(void);
uint64_t benchmark_get_memory_usage(void);

#endif // BHARAT_BENCHMARK_H
