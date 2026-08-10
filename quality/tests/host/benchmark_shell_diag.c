#include <stdio.h>
#include <assert.h>
#include <time.h>
#include "shell_dispatch.h"
#include "shell_session.h"

// Constant uptime backend
static int constant_uptime(uint64_t* up) {
    if (up) *up = 1000;
    return 0;
}

// Simulated active spin loop (original cmd_diag_run behavior)
static int diag_run_spin_loop(char* out, size_t out_len) {
    volatile unsigned long i;
    for (i = 0; i < 1000000ul; ++i) { }
    if (out && out_len > 10) {
        snprintf(out, out_len, "diagnostics=ok");
    }
    return 0;
}

// Optimized no-op callback
static int diag_run_optimized(char* out, size_t out_len) {
    if (out && out_len > 10) {
        snprintf(out, out_len, "diagnostics=ok");
    }
    return 0;
}

int main(void) {
    shell_session_t session;
    shell_argv_t argv;
    shell_backend_api_t backend = *shell_default_backend();

    backend.get_uptime_ms = constant_uptime;
    shell_session_init(&session, SHELL_MODE_DEV, SHELL_CAP_DIAG);

    argv.count = 2;
    argv.tokens[0] = "diag";
    argv.tokens[1] = "run";

    struct timespec start, end;
    double elapsed_spin, elapsed_optimized;
    const int iterations = 1000;

    // 1. Measure Spin Loop Baseline
    backend.diag_run = diag_run_spin_loop;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        shell_response_t r = shell_dispatch(&session, &backend, &argv);
        assert(r.code == SHELL_RC_OK);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_spin = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // 2. Measure Optimized Code
    backend.diag_run = diag_run_optimized;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        shell_response_t r = shell_dispatch(&session, &backend, &argv);
        assert(r.code == SHELL_RC_OK);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_optimized = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("=== Shell Diag Active Spin Removal Benchmark ===\n");
    printf("Iterations: %d\n", iterations);
    printf("Baseline Spin Loop Uptime: %.6f seconds\n", elapsed_spin);
    printf("Optimized Backend-driven:  %.6f seconds\n", elapsed_optimized);
    if (elapsed_spin > 0.0) {
        double speedup = (elapsed_spin - elapsed_optimized) / elapsed_spin * 100.0;
        printf("Measured CPU Time Reduction: %.2f%%\n", speedup);
    }
    printf("=================================================\n");

    return 0;
}
