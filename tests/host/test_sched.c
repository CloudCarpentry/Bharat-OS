#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Mock kernel definitions before including sched.c
#define TESTING 1

#include "../../kernel/include/hal/hal.h"
#include "../../kernel/include/sched/sched.h"
#include "../../kernel/include/bharat/cpu_local.h"
#include "../../kernel/include/slab.h"
#include "../../kernel/include/sched/ai_sched.h"
#include "../../kernel/include/ipc_async.h"
#include "../../kernel/include/arch/arch_ext_state.h"
#include "../../kernel/include/mm/mm_aspace_switch.h"
#include "../../kernel/include/capability.h"
#include "../../kernel/include/core/multikernel.h"

// --- Global variables for mocks ---
#define MAX_SUPPORTED_CORES 8U
cpu_local_t g_cpu_locals[MAX_CPUS];

// --- Mocking basic kernel functions ---

// Removed hal_cpu_get_id, hal_cpu_disable_interrupts, hal_cpu_enable_interrupts,
// hal_send_ipi_payload, kernel_panic, mk_get_channel, and mk_send_message
// because they are now provided by host_stubs.c.

void hal_cpu_halt(void) {
    // Should not be called in test
    // We mock this by simply returning to avoid infinite loop in sched_idle_task test
}

void* kmalloc(size_t size) {
    return malloc(size);
}

void kfree(void* ptr) {
    free(ptr);
}

address_space_t* mm_create_address_space(void) {
    return (address_space_t*)malloc(sizeof(address_space_t));
}

int aspace_destroy(address_space_t* aspace) {
    free(aspace);
    return 0;
}

void mm_switch_active_aspace(uint32_t core_id, address_space_t* prev, address_space_t* next) {
    (void)core_id; (void)prev; (void)next;
}

int cap_table_init_for_process(bh_process_t* proc) {
    (void)proc;
    return 0;
}

void cap_table_destroy(capability_table_t* table) {
    (void)table;
}

void arch_prepare_initial_context(cpu_context_t* ctx, void (*entry_point)(void), uint64_t stack_top) {
    (void)ctx; (void)entry_point; (void)stack_top;
}

int arch_ext_state_thread_init(bh_thread_t* thread) {
    (void)thread;
    return 0;
}

void arch_ext_state_thread_destroy(bh_thread_t* thread) {
    (void)thread;
}

void arch_ext_state_save(bh_thread_t* thread) {
    (void)thread;
}

void arch_ext_state_restore(bh_thread_t* thread) {
    (void)thread;
}

void arch_context_switch(cpu_context_t* prev, cpu_context_t* next) {
    (void)prev; (void)next;
}

void fv_secure_context_switch(void* next) {
    (void)next;
}

void ai_sched_init_context(ai_sched_context_t* ctx) {
    (void)ctx;
}

void ai_sched_collect_sample(ai_sched_context_t* ctx, uint64_t slice, uint64_t consumed, uint32_t run_queue_depth, uint32_t switches) {
    (void)ctx; (void)slice; (void)consumed; (void)run_queue_depth; (void)switches;
}

void ipc_async_check_timeouts(uint64_t current_ticks) {
    (void)current_ticks;
}

void deg_block_member(bh_thread_t* thread, uint32_t reason) {
    (void)thread; (void)reason;
}

void vmm_process_local_urpc_messages(uint32_t core_id) {
    (void)core_id;
}

void vm_debug_validate_active_tracking(void) {
}

// Ensure the inclusion compiles our actual object code
#include "../../kernel/src/sched/sched.c"

// --- Tests ---

void test_sched_init() {
    sched_init();

    // In thread_create for the idle threads, g_free_thread_head and g_free_process_head are advanced!
    // So checking g_free_thread_head == 0 will fail, since threads were allocated during sched_init.

    // Check if idle task is correctly created on core 0
    bh_thread_t* idle = g_cpu_locals[0].runqueue.idle_thread;
    assert(idle != NULL);
    assert(idle->priority == 0);
    assert(idle->bound_core_id == 0);

    printf("test_sched_init passed\n");
}

void mock_task_entry() { }

void test_thread_create_and_enqueue() {
    sched_init(); // Reset state

    bh_process_t* proc = process_create("test_proc");
    assert(proc != NULL);

    bh_thread_t* thread = thread_create(proc, mock_task_entry);
    assert(thread != NULL);
    assert(thread->state == THREAD_STATE_READY);
    assert(thread->priority == 1); // Default
    assert(thread->process == proc);

    // thread_create calls sched_enqueue implicitly if entry_point is not sched_idle_task
    thread_slot_t* slot = sched_find_thread_slot_by_tid(thread->thread_id);
    assert(slot != NULL);
    assert(slot->is_on_runqueue == 1);

    // Actually when we initialize, the run queue already has 1 monitor thread. Let's check runnable_count is 2 now
    // Wait, the idle thread doesn't increment runnable_count, but the monitor thread does

    assert((g_cpu_locals[0].runqueue.ready_bitmap & (1U << thread->priority)) != 0);

    printf("test_thread_create_and_enqueue passed\n");
}

void test_thread_pick_next_ready() {
    sched_init();
    bh_process_t* proc = process_create("test_proc");

    // Drain the run queue initially, as sched_init adds a monitor task
    while (g_cpu_locals[0].runqueue.runnable_count > 0) {
        sched_pick_next_ready(0);
    }

    // Create higher priority thread (detached prevents auto-enqueue and avoids double-counting)
    bh_thread_t* t1 = thread_create_detached(proc, mock_task_entry);
    t1->priority = 5;
    sched_enqueue(t1, 0); // Re-enqueue with new priority

    // Create lower priority thread
    bh_thread_t* t2 = thread_create_detached(proc, mock_task_entry);
    t2->priority = 2;
    sched_enqueue(t2, 0);

    // Invariant: runnable_count tracks how many tasks are ON the runqueue (not currently running)
    assert(g_cpu_locals[0].runqueue.runnable_count == 2);

    // Policy default is PRIORITY
    bh_thread_t* picked = sched_pick_next_ready(0);
    assert(picked == t1); // Priority 5 is higher

    thread_slot_t* slot1 = sched_find_thread_slot_by_tid(t1->thread_id);
    assert(slot1->is_on_runqueue == 0); // Popped off queue during pick
    // Running thread (t1) is no longer on the run queue
    assert(g_cpu_locals[0].runqueue.runnable_count == 1);

    picked = sched_pick_next_ready(0);
    assert(picked == t2); // Priority 2 is next

    printf("test_thread_pick_next_ready passed\n");
}

void test_sched_mark_terminated_and_reap() {
    sched_init();
    bh_process_t* proc = process_create("test_proc");
    bh_thread_t* t1 = thread_create(proc, mock_task_entry);

    assert(t1->state == THREAD_STATE_READY);

    int ret = sched_mark_thread_terminated(t1);
    assert(ret == 0);
    assert(t1->state == THREAD_STATE_TERMINATED);

    // It should have been removed from runqueue when marked terminated
    thread_slot_t* slot = sched_find_thread_slot_by_tid(t1->thread_id);
    assert(slot->is_on_runqueue == 0);

    assert(slot->reap_pending == 1);
    assert(g_reap_head != UINT32_MAX);

    // Perform reaping
    sched_reap_terminated_threads();
    assert(slot->reap_pending == 0);
    assert(slot->in_use == 0); // Slot freed

    printf("test_sched_mark_terminated_and_reap passed\n");
}

int main() {
    test_sched_init();
    test_thread_create_and_enqueue();
    test_thread_pick_next_ready();
    test_sched_mark_terminated_and_reap();

    printf("All host sched tests passed.\n");
    return 0;
}
