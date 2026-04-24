#include "bench.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static int sched_setup(void **ctx) {
    // Mock runqueue struct
    void *rq = malloc(4096);
    *ctx = rq;
    return rq ? 0 : -1;
}

static int sched_run_context_switch(void *ctx, uint64_t iterations) {
    // Mock run representing thread ping-pong context switch overhead
    void *rq = ctx;
    uint32_t current_thread = 0;
    for (uint64_t i = 0; i < iterations; i++) {
        // Mock save context / load context
        uint32_t next_thread = (current_thread + 1) % 2;
        ((uint32_t*)rq)[0] = next_thread;
        current_thread = next_thread;
    }
    return 0;
}

static void sched_teardown(void *ctx) {
    free(ctx);
}

REGISTER_BENCHMARK(sched_ctx_switch, "scheduler", BENCH_LEVEL_B1, 1000000, 100000, sched_setup, sched_run_context_switch, sched_teardown);
