#include <assert.h>
#include <stdio.h>

#include "../kernel/include/advanced/ai_kernel_bridge.h"
#include "../kernel/include/sched.h"

#include "../kernel/include/mm.h"
#include "../kernel/include/numa.h"
#include "../kernel/include/ipc_endpoint.h"

address_space_t* mm_create_address_space(void) {
    static address_space_t as = { .root_table = 1U };
    return &as;
}

static void thread_entry(void) {}

int main(void) {
    sched_init();

    kprocess_t* proc = process_create("ai");
    assert(proc != NULL);

    kthread_t* t = thread_create(proc, thread_entry);
    assert(t != NULL);
    kthread_t* t2 = thread_create(proc, thread_entry);
    assert(t2 != NULL);

    ai_suggestion_t priority = {
        .action = AI_ACTION_ADJUST_PRIORITY,
        .target_id = (uint32_t)t2->thread_id,
        .value = 7U,
    };

    assert(ai_kernel_apply_suggestion(&priority) == 0);
    sched_on_timer_tick();
    kthread_t* updated = sched_find_thread_by_id(t2->thread_id);
    assert(updated != NULL);
    assert(updated->priority == 7U);

    sched_yield();
    assert(sched_current_thread() != NULL);

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
    sched_on_timer_tick();
    kthread_t* migrated = sched_find_thread_by_id(t->thread_id);
    assert(migrated != NULL);
    assert(migrated->preferred_numa_node == 1U);

    capability_table_t* caps = (capability_table_t*)proc->security_sandbox_ctx;
    assert(caps != NULL);

    uint32_t send_cap = 0U;
    uint32_t recv_cap = 0U;
    assert(ai_kernel_create_governor_endpoint(caps, &send_cap, &recv_cap) == 0);

    ai_suggestion_t queued = {
        .action = AI_ACTION_ADJUST_PRIORITY,
        .target_id = (uint32_t)t->thread_id,
        .value = 9U,
    };
    assert(ipc_endpoint_send(caps, send_cap, &queued, sizeof(queued)) == IPC_OK);
    assert(ai_kernel_ingest_suggestion_ipc(caps, recv_cap) == 0);

    sched_on_timer_tick();
    assert(t->priority == 9U);

    kernel_telemetry_t telemetry = {0};
    assert(ai_kernel_collect_telemetry(&telemetry) == 0);
    assert(telemetry.ipc_latency_ns > 0U);
    assert(telemetry.context_switches > 0U);

    printf("AI kernel bridge tests passed.\n");
    return 0;
}
