#include "bench.h"
#include "bench_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern bench_case_t __start_benchmarks;
extern bench_case_t __stop_benchmarks;

static bench_runner_config_t g_config;
static int g_tests_run = 0;
static int g_tests_failed = 0;

void bench_runner_init(const bench_runner_config_t *config) {
    if (config) {
        g_config = *config;
    } else {
        g_config.filter_group = NULL;
        g_config.filter_name = NULL;
        g_config.format = OUTPUT_FORMAT_CONSOLE;
        g_config.override_iterations = 0;
    }
}

static int should_run(const bench_case_t *bench) {
    if (g_config.filter_group && strcmp(bench->group, g_config.filter_group) != 0) {
        return 0;
    }
    if (g_config.filter_name && strcmp(bench->name, g_config.filter_name) != 0) {
        return 0;
    }
    return 1;
}

static void print_json_result(const bench_case_t *bench, bench_metrics_t *metrics, uint64_t iters) {
    printf("{\n");
    printf("  \"name\": \"%s\",\n", bench->name);
    printf("  \"group\": \"%s\",\n", bench->group);
    printf("  \"iterations\": %llu,\n", (unsigned long long)iters);
    printf("  \"ns_per_op\": %llu,\n", (unsigned long long)metrics->ns_per_op);
    printf("  \"total_ns\": %llu,\n", (unsigned long long)metrics->total_ns);
    printf("  \"ops_per_sec\": %llu\n", (unsigned long long)metrics->ops_per_sec);
    printf("}\n");
}

static void print_console_result(const bench_case_t *bench, bench_metrics_t *metrics, uint64_t iters) {
    printf("[BENCH] %-20s | Group: %-10s | Iters: %8llu | ns/op: %8llu | ops/sec: %10llu\n",
           bench->name, bench->group, (unsigned long long)iters,
           (unsigned long long)metrics->ns_per_op,
           (unsigned long long)metrics->ops_per_sec);
}

int bench_runner_execute_all(void) {
    bench_case_t *bench;
    bench_metrics_t metrics;
    void *ctx = NULL;

    if (g_config.format == OUTPUT_FORMAT_CONSOLE) {
        printf("=========================================================================================\n");
        printf(" Bharat-OS Benchmark Runner\n");
        printf("=========================================================================================\n");
    }

    for (bench = &__start_benchmarks; bench < &__stop_benchmarks; bench++) {
        if (!should_run(bench)) continue;

        g_tests_run++;

        uint64_t iters = g_config.override_iterations ? g_config.override_iterations : bench->default_iterations;

        if (bench->setup) {
            if (bench->setup(&ctx) != 0) {
                if (g_config.format == OUTPUT_FORMAT_CONSOLE) {
                    printf("[ERROR] Setup failed for %s\n", bench->name);
                }
                g_tests_failed++;
                continue;
            }
        }

        // Warmup (Not timed)
        if (bench->warmup_iterations > 0 && bench->run) {
            bench->run(ctx, bench->warmup_iterations);
        }

        // Timed run
        memset(&metrics, 0, sizeof(metrics));
        uint64_t start_time = bench_clock_now_ns();

        if (bench->run) {
            int res = bench->run(ctx, iters);
            if (res != 0) {
                if (g_config.format == OUTPUT_FORMAT_CONSOLE) {
                    printf("[ERROR] Run failed for %s\n", bench->name);
                }
                g_tests_failed++;
                goto cleanup;
            }
        }

        uint64_t end_time = bench_clock_now_ns();
        metrics.total_ns = end_time - start_time;

        if (iters > 0) {
            metrics.ns_per_op = metrics.total_ns / iters;
            if (metrics.total_ns > 0) {
                metrics.ops_per_sec = (iters * 1000000000ULL) / metrics.total_ns;
            }
        }

        if (g_config.format == OUTPUT_FORMAT_JSON) {
            print_json_result(bench, &metrics, iters);
        } else if (g_config.format == OUTPUT_FORMAT_CONSOLE) {
            print_console_result(bench, &metrics, iters);
        }

cleanup:
        if (bench->teardown) {
            bench->teardown(ctx);
        }
    }

    return g_tests_failed > 0 ? -1 : 0;
}

void bench_runner_print_summary(void) {
    if (g_config.format == OUTPUT_FORMAT_CONSOLE) {
        printf("=========================================================================================\n");
        printf(" Summary: %d executed, %d failed\n", g_tests_run, g_tests_failed);
        printf("=========================================================================================\n");
    }
}
