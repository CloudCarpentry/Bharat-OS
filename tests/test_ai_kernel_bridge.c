
#include <assert.h>
#include <stdio.h>

#include "../kernel/include/advanced/ai_kernel_bridge.h"
#include "../kernel/include/sched.h"

#include "../kernel/include/mm.h"
#include "../kernel/include/numa.h"
#include "../kernel/include/ipc_endpoint.h"

static void thread_entry(void) {}

int main(void) {
    sched_init();

    kprocess_t proc;
    proc.process_id = 1;

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

    kernel_telemetry_t telemetry = {0};
    assert(ai_kernel_collect_telemetry(&telemetry) == 0);
    assert(telemetry.ipc_latency_ns > 0U);

    printf("AI kernel bridge tests passed.\n");
    return 0;
}

#include "../kernel/include/slab.h"
#include <stdlib.h>
#include <string.h>

kcache_t* kcache_create(const char* name, size_t size) {
    kcache_t* c = malloc(sizeof(kcache_t));
    if(c) {
        c->object_size = size;
        c->name = name;
    }
    return c;
}
void* kcache_alloc(kcache_t* cache) {
    if(!cache) return NULL;
    return malloc(cache->object_size);
}
void kcache_free(kcache_t* cache, void* obj) {
}



int cap_table_init_for_process(kprocess_t* process) {
    (void)process;
    return 0;
}
