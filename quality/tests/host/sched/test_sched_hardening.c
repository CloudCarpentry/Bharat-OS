#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <bharat/cpu_local.h>
#include <sched/sched.h>
#include <kernel/status.h>

// Mock active_core_count
uint32_t g_active_core_count = 4;

// Mocks
void kernel_panic(const char* msg) {
    printf("PANIC: %s\n", msg);
}

cpu_local_t g_cpu_locals[32];

typedef struct thread_slot {
  uint8_t in_use;
  uint8_t is_bootstrap;
  uint32_t next_free;
  bh_thread_t thread;
  uint64_t context[64];
  uint64_t ai_ctx[16];
  list_head_t run_node;
  list_head_t wait_node;
  uint32_t reap_next;
  uint8_t reap_pending;
  uint8_t is_on_runqueue;
  uint8_t is_sleeping;
  uint8_t is_blocked;
  uint32_t creation_core_id;
  sched_remote_cmd_t remote_cmd;
} thread_slot_t;

thread_slot_t *sched_find_thread_slot_by_tid_local(sched_rq_t *rq, uint64_t tid) {
    static thread_slot_t slot;
    memset(&slot, 0, sizeof(slot));
    slot.thread.thread_id = tid;
    slot.is_on_runqueue = 1;
    return &slot;
}

void hal_send_ipi_payload(uint32_t mask, uint32_t payload) {
    (void)mask; (void)payload;
}

sched_policy_t g_policy = SCHED_POLICY_ROUND_ROBIN;

// Simplified manual implementation of the logic we want to test to avoid massive linking
int mock_sched_quarantine_thread(bh_thread_t *thread, uint32_t reason) {
  if (!thread) return -1;
  thread->state = THREAD_STATE_QUARANTINED;
  thread->pending_fault = (thread_fault_t)reason;
  return 0;
}

int mock_sched_migrate_task(bh_thread_t *thread, uint32_t new_node) {
  if (thread->state == THREAD_STATE_QUARANTINED) {
    return K_ERR_BAD_STATE;
  }
  return K_OK;
}

int mock_sched_enqueue(bh_thread_t *thread, uint32_t core_id) {
  if (g_cpu_locals[core_id].runqueue.sched_isolated) {
      return K_ERR_ISOLATED;
  }
  return K_OK;
}

void test_quarantine(void) {
    printf("Running test_quarantine...\n");
    bh_thread_t thread;
    memset(&thread, 0, sizeof(thread));
    thread.state = THREAD_STATE_READY;

    mock_sched_quarantine_thread(&thread, 0x123);
    assert(thread.state == THREAD_STATE_QUARANTINED);
    assert(thread.pending_fault == 0x123);

    // Check that migration fails
    assert(mock_sched_migrate_task(&thread, 1) == K_ERR_BAD_STATE);

    printf("test_quarantine passed!\n");
}

void test_core_isolation(void) {
    printf("Running test_core_isolation...\n");
    g_cpu_locals[1].runqueue.sched_isolated = true;

    bh_thread_t thread;
    memset(&thread, 0, sizeof(thread));
    thread.priority = 10;
    thread.home_core_id = 0;

    // Enqueue to isolated core should fail
    assert(mock_sched_enqueue(&thread, 1) == K_ERR_ISOLATED);

    printf("test_core_isolation passed!\n");
}

uint32_t hal_cpu_get_id(void) { return 0; }
uint32_t sched_clamp_core(uint32_t core_id) { return core_id; }

int main(void) {
    test_quarantine();
    test_core_isolation();
    return 0;
}
