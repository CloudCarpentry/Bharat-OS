
#include <assert.h>
#include <stdio.h>

#include "../kernel/include/advanced/ai_kernel_bridge.h"
#include "../kernel/include/sched.h"

#include "../kernel/include/mm.h"
#include "../kernel/include/numa.h"
#include "../kernel/include/ipc_endpoint.h"

static void thread_entry(void) {}

uint8_t g_per_core_stacks[32][16384];

void isr0(void) {} void isr1(void) {} void isr2(void) {} void isr3(void) {}
void isr4(void) {} void isr5(void) {} void isr6(void) {} void isr7(void) {}
void isr8(void) {} void isr9(void) {} void isr10(void) {} void isr11(void) {}
void isr12(void) {} void isr13(void) {} void isr14(void) {} void isr15(void) {}
void isr16(void) {} void isr17(void) {} void isr18(void) {} void isr19(void) {}
void isr20(void) {} void isr21(void) {} void isr22(void) {} void isr23(void) {}
void isr24(void) {} void isr25(void) {} void isr26(void) {} void isr27(void) {}
void isr28(void) {} void isr29(void) {} void isr30(void) {} void isr31(void) {}
void isr32(void) {} void isr128(void) {}

int main(void) {
    sched_init();

    kprocess_t proc;
    proc.process_id = 1;
    cap_table_init_for_process(&proc); // Ensure caps are initialized

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

    capability_table_t* caps = (capability_table_t*)proc.security_sandbox_ctx;
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

int hal_vmm_init_root(address_space_t *as) {
    (void)as;
    return 0;
}

int hal_vmm_map_page(address_space_t *as, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)as;
    (void)vaddr;
    (void)paddr;
    (void)flags;
    return 0;
}

int hal_vmm_unmap_page(address_space_t *as, virt_addr_t vaddr) {
    (void)as;
    (void)vaddr;
    return 0;
}

int hal_vmm_setup_address_space(address_space_t *as) {
    (void)as;
    return 0;
}

int hal_vmm_get_mapping(address_space_t *as, virt_addr_t vaddr, phys_addr_t *out_paddr, uint32_t *out_flags) {
    (void)as;
    (void)vaddr;
    (void)out_paddr;
    (void)out_flags;
    return -1;
}

int hal_vmm_update_mapping(address_space_t *as, virt_addr_t vaddr, uint32_t new_flags) {
    (void)as;
    (void)vaddr;
    (void)new_flags;
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



// No longer stub cap_table_init_for_process because we pull in real capability.c
