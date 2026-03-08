#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "benchmark/benchmark.h"
#include "sched.h"

#include "../kernel/include/ipc_async.h"

void ipc_async_check_timeouts(uint64_t current_ticks) {
    (void)current_ticks;
}

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

// Stubs for NUMA page migration dependencies
phys_addr_t pmm_alloc_pages_colored(int order, uint32_t preferred_numa_node, uint32_t flags, mm_color_config_t *color_config) {
    (void)order; (void)preferred_numa_node; (void)flags; (void)color_config;
    return 0;
}
phys_addr_t mm_alloc_pages_order(int order, uint32_t preferred_numa_node, uint32_t flags) {
    (void)order; (void)preferred_numa_node; (void)flags;
    return 0;
}
void tlb_shootdown(virt_addr_t vaddr) {
    (void)vaddr;
}
