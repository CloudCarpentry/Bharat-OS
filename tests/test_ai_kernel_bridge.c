#include <assert.h>
#include <stdio.h>

#include "../kernel/include/advanced/ai_kernel_bridge.h"
#include "../kernel/include/sched.h"

#include "../kernel/include/mm.h"
#include "../kernel/include/numa.h"

address_space_t* mm_create_address_space(void) {
    static address_space_t as = { .root_table = 1U };
    return &as;
}

int cap_table_init_for_process(kprocess_t* process) {
    (void)process;
    return 0;
}

static void thread_entry(void) {}

int main(void) {
    sched_init();

    kprocess_t proc = {0};
    proc.process_id = 1U;

    kthread_t* t = thread_create(&proc, thread_entry);
    assert(t != NULL);
    kthread_t* t2 = thread_create(&proc, thread_entry);
    assert(t2 != NULL);

    ai_suggestion_t priority = {
        .action = AI_ACTION_ADJUST_PRIORITY,
        .target_id = (uint32_t)t2->thread_id,
        .value = 7U,
    };

    assert(ai_kernel_apply_suggestion(&priority) == 0);
    kthread_t* updated = sched_find_thread_by_id(t2->thread_id);
    assert(updated != NULL);
    assert(updated->priority == 7U);

    sched_yield();
    assert(sched_current_thread() == updated);

    assert(numa_discover_topology() == 0);
    assert(numa_set_node_descriptor(1U, 0x1000U, 0x1000U, 1U) == 0);

    ai_suggestion_t invalid_migrate = {
        .action = AI_ACTION_MIGRATE_TASK,
        .target_id = (uint32_t)t->thread_id,
        .value = 7U,
    };
    assert(ai_kernel_apply_suggestion(&invalid_migrate) != 0);

    ai_suggestion_t migrate = {
        .action = AI_ACTION_MIGRATE_TASK,
        .target_id = (uint32_t)t->thread_id,
        .value = 1U,
    };

    assert(ai_kernel_apply_suggestion(&migrate) == 0);
    kthread_t* migrated = sched_find_thread_by_id(t->thread_id);
    assert(migrated != NULL);
    assert(migrated->preferred_numa_node == 1U);

    sched_yield();
    kernel_telemetry_t telemetry = {0};
    assert(ai_kernel_collect_telemetry(&telemetry) == 0);
    assert(telemetry.ipc_latency_ns > 0U);

    printf("AI kernel bridge tests passed.\n");
    return 0;
}
