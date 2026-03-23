#ifndef BHARAT_OS_BENCH_RUNNER_H
#define BHARAT_OS_BENCH_RUNNER_H

#include "bench.h"
#include "bench_metrics.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OUTPUT_FORMAT_CONSOLE,
    OUTPUT_FORMAT_JSON,
    OUTPUT_FORMAT_CSV
} output_format_t;

/**
 * Runner Configuration
 */
typedef struct {
    const char *filter_group;
    const char *filter_name;
    output_format_t format;
    uint64_t override_iterations;
} bench_runner_config_t;

/**
 * High-resolution timer wrapper (Backend specific)
 */
uint64_t bench_clock_now_ns(void);

/**
 * Core Runner API
 */
void bench_runner_init(const bench_runner_config_t *config);
int bench_runner_execute_all(void);
void bench_runner_print_summary(void);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_OS_BENCH_RUNNER_H
