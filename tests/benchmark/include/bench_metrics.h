#ifndef BHARAT_OS_BENCH_METRICS_H
#define BHARAT_OS_BENCH_METRICS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Standard metrics collected during a benchmark run
 */
typedef struct {
    uint64_t total_ns;
    uint64_t ns_per_op;
    uint64_t ops_per_sec;
    uint64_t bytes_per_sec;

    // Distribution metrics
    uint64_t min_ns;
    uint64_t max_ns;
    uint64_t median_ns;
    uint64_t p95_ns;
    uint64_t p99_ns;
    uint64_t stddev_ns;

    // OS-specific events (can be populated if supported by the backend)
    uint64_t allocation_failures;
    uint64_t retries;
    uint64_t lock_contention;
    uint64_t context_switches;
    uint64_t migrations;
    uint64_t cache_misses;
    uint64_t queue_depth;
    uint64_t dropped_packets;
} bench_metrics_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_OS_BENCH_METRICS_H
