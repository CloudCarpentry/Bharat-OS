#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "benchmark/benchmark.h"
#include "sched.h"

// Mock device manager things needed for compilation
uint32_t g_num_drivers = 0;
uint32_t g_num_windows = 0;

void dummy_entry(void) {
    // dummy
}

void test_sched_benchmark(void) {
    sched_init();

    kprocess_t* proc = process_create("bench_proc");
    assert(proc != NULL);

    const int ITERATIONS = 10000;

    benchmark_ctx_t ctx;
    benchmark_start(&ctx, "Thread Creation/Destruction", BENCHMARK_LEVEL_0_REF, ITERATIONS);

    for (int i = 0; i < ITERATIONS; i++) {
        kthread_t* thread = thread_create(proc, dummy_entry);
        assert(thread != NULL);
        thread_destroy(thread);
    }

    benchmark_stop(&ctx);

    benchmark_result_t result;
    benchmark_record(&ctx, &result);
    benchmark_print(&result, ctx.name);


    // Benchmark thread lookups
    kthread_t* threads[100];
    for (int i = 0; i < 100; i++) {
        threads[i] = thread_create(proc, dummy_entry);
    }

    benchmark_start(&ctx, "Thread ID Lookup (100 threads)", BENCHMARK_LEVEL_0_REF, ITERATIONS);
    for (int i = 0; i < ITERATIONS; i++) {
        // Find by pseudo-random thread ID
        uint64_t target_id = threads[i % 100]->thread_id;
        kthread_t* found = sched_find_thread_by_id(target_id);
        assert(found != NULL);
    }
    benchmark_stop(&ctx);
    benchmark_record(&ctx, &result);
    benchmark_print(&result, ctx.name);

    for (int i = 0; i < 100; i++) {
        thread_destroy(threads[i]);
    }
}

int main(void) {
    printf("Running Scheduler Benchmarks...\n");
    test_sched_benchmark();
    return 0;
}
