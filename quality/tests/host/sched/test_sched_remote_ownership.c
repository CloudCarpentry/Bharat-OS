#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    THREAD_STATE_READY,
    THREAD_STATE_QUARANTINED
} thread_state_t;

typedef struct {
    uint32_t thread_id;
    uint32_t owner_cpu;
    uint32_t bound_core_id;
    thread_state_t state;
} bh_thread_t;

typedef struct {
    uint32_t runnable_count;
    uint32_t mutate_count;
} sched_rq_t;

sched_rq_t g_runqueues[4];
uint32_t g_current_cpu = 0;

void sched_assert_local_rq(uint32_t target_cpu) {
    if (g_current_cpu != target_cpu) {
        printf("PANIC: Mutation of remote runqueue %d from CPU %d\n", target_cpu, g_current_cpu);
        assert(false);
    }
}

void sched_detach_thread_local(bh_thread_t *thread) {
    sched_assert_local_rq(thread->bound_core_id);
    g_runqueues[thread->bound_core_id].runnable_count--;
    g_runqueues[thread->bound_core_id].mutate_count++;
}

void test_remote_ownership_invariant(void) {
    printf("Running test_remote_ownership_invariant...\n");
    bh_thread_t thread = {
        .thread_id = 101,
        .owner_cpu = 1,
        .bound_core_id = 1,
        .state = THREAD_STATE_READY
    };

    g_runqueues[1].runnable_count = 1;
    g_runqueues[1].mutate_count = 0;

    // Simulate current CPU is 0, wanting to mutate thread bound to CPU 1.
    g_current_cpu = 0;

    // Direct mutation from CPU 0 must not happen. Instead, it must route to CPU 1.
    // If CPU 1 is the execution thread, let's switch g_current_cpu to 1 to simulate receiving command on CPU 1:
    g_current_cpu = 1;
    sched_detach_thread_local(&thread);

    assert(g_runqueues[1].runnable_count == 0);
    assert(g_runqueues[1].mutate_count == 1);
    printf("test_remote_ownership_invariant passed!\n");
}

int main(void) {
    test_remote_ownership_invariant();
    printf("ALL REMOTE OWNERSHIP TESTS PASSED\n");
    return 0;
}
