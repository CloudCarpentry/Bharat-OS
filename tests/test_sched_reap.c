#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/sched.h"
#include "../kernel/include/ipc_async.h"

void arch_post_switch(void);

static uint32_t g_mock_core_id = 0;
static uint32_t g_aspace_destroy_calls = 0;
static address_space_t g_as = {.root_pt = 0x2000U};

void ipc_async_check_timeouts(uint64_t current_ticks) { (void)current_ticks; }
uint32_t hal_cpu_get_id(void) { return g_mock_core_id; }

address_space_t *mm_create_address_space(void) { return &g_as; }
int aspace_destroy(address_space_t *aspace) {
  if (aspace) {
    g_aspace_destroy_calls++;
  }
  return 0;
}

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
  (void)preferred_numa_node;
  return 0;
}
void mm_free_page(phys_addr_t page) { (void)page; }
void vm_debug_validate_active_tracking(void) {}
void arch_context_switch(cpu_context_t *prev, cpu_context_t *next) {
  (void)prev;
  (void)next;
  arch_post_switch();
}

void dummy_thread(void) {}

int main(void) {
  sched_init();

  kprocess_t *p = process_create("reap_proc");
  assert(p != NULL);
  kthread_t *t = thread_create(p, dummy_thread);
  assert(t != NULL);

  ai_suggestion_t kill = {
      .action = AI_ACTION_KILL_TASK,
      .target_id = (uint32_t)t->thread_id,
      .value = 0,
  };
  assert(sched_ai_apply_suggestion(&kill) == 0);
  assert(sched_find_thread_by_id(t->thread_id) != NULL);
  assert(t->state == THREAD_STATE_TERMINATED);

  sched_reschedule();
  assert(sched_find_thread_by_id(t->thread_id) == NULL);

  assert(process_destroy(p) == 0);
  assert(g_aspace_destroy_calls >= 1U);

  kprocess_t *p2 = process_create("live_proc");
  assert(p2 != NULL);
  kthread_t *t2 = thread_create(p2, dummy_thread);
  assert(t2 != NULL);
  assert(process_destroy(p2) != 0);

  assert(sched_sys_thread_destroy(t2->thread_id) == 0);
  assert(sched_find_thread_by_id(t2->thread_id) != NULL);
  sched_on_timer_tick();
  assert(sched_find_thread_by_id(t2->thread_id) == NULL);
  assert(process_destroy(p2) == 0);

  printf("test_sched_reap passed\n");
  return 0;
}
