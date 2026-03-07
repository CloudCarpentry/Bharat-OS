#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/sched.h"

static address_space_t g_as = { .root_table = 0x1000U };

address_space_t* mm_create_address_space(void) {
    return &g_as;
}

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
    (void)preferred_numa_node;
    return 0;
}

void mm_free_page(phys_addr_t page) {
    (void)page;
}

void thread_a(void) {}
void thread_b(void) {}

int main(void) {
    sched_init();

    kprocess_t* p = process_create("init");
    assert(p != NULL);

    kthread_t* t1 = thread_create(p, thread_a);
    kthread_t* t2 = thread_create(p, thread_b);
    assert(t1 != NULL && t2 != NULL);

    // First tick should dispatch first ready thread.
    sched_on_timer_tick();
    assert(sched_current_thread() != NULL);

    // Force a yield and ensure scheduler can switch to another ready thread.
    kthread_t* before = sched_current_thread();
    sched_yield();
    kthread_t* after = sched_current_thread();
    assert(after != NULL);
    assert(after != before);

    // Syscall façade coverage
    uint64_t tid = 0;
    assert(sched_sys_thread_create(p, thread_a, &tid) == 0);
    assert(tid != 0);
    assert(sched_sys_thread_destroy(tid) == 0);

    assert(thread_destroy(t1) == 0);
    assert(thread_destroy(t2) == 0);

    printf("Scheduler tests passed.\n");
    return 0;
}
