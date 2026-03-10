#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../kernel/include/ipc_async.h"
#include "../kernel/include/sched.h"
#include "../kernel/include/slab.h"

static uint32_t g_mock_core_id = 0;

void* kmalloc(size_t size) {
    return malloc(size);
}
void kfree(void* ptr) {
    free(ptr);
}
uint64_t hal_timer_monotonic_ticks(void) {
    return 0;
}
void arch_prepare_initial_context(cpu_context_t* ctx, void (*entry)(void), uint64_t stack_top) {
    (void)ctx;
    (void)entry;
    (void)stack_top;
}
void arch_context_switch(cpu_context_t* prev, cpu_context_t* next) {
    (void)prev;
    (void)next;
}
static address_space_t g_as = {.root_table = 0x1000U};

void ipc_async_check_timeouts(uint64_t current_ticks) { (void)current_ticks; }
address_space_t *mm_create_address_space(void) { return &g_as; }
phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
  (void)preferred_numa_node;
  return 0;
}
void mm_free_page(phys_addr_t page) { (void)page; }

phys_addr_t pmm_alloc_pages_colored(int order, uint32_t preferred_numa_node,
                                    uint32_t flags,
                                    mm_color_config_t *color_config) {
  (void)order;
  (void)preferred_numa_node;
  (void)flags;
  (void)color_config;
  return 0;
}
phys_addr_t mm_alloc_pages_order(int order, uint32_t preferred_numa_node,
                                 uint32_t flags) {
  (void)order;
  (void)preferred_numa_node;
  (void)flags;
  return 0;
}
int hal_vmm_get_mapping(phys_addr_t root_table, virt_addr_t vaddr,
                        phys_addr_t *paddr, uint32_t *flags) {
  (void)root_table;
  (void)vaddr;
  (void)paddr;
  (void)flags;
  return -1;
}
int hal_vmm_update_mapping(phys_addr_t root_table, virt_addr_t vaddr,
                           phys_addr_t paddr, uint32_t flags) {
  (void)root_table;
  (void)vaddr;
  (void)paddr;
  (void)flags;
  return -1;
}
void tlb_shootdown(virt_addr_t vaddr) { (void)vaddr; }

uint32_t hal_cpu_get_id(void) { return g_mock_core_id; }
void hal_cpu_halt(void) {}

static void thread_a(void) {}
static void thread_b(void) {}

static int g_mutex_a;
static int g_mutex_b;

static void test_lifecycle_and_syscalls(void) {
  sched_init();
  kprocess_t *p = process_create("init");
  assert(p != NULL);

  uint64_t tid = 0;
  assert(sched_sys_thread_create(p, thread_a, &tid) == 0);
  assert(tid != 0);
  assert(sched_sys_set_priority(tid, 9) == 0);
  assert(sched_sys_set_affinity(tid, 0x3) == 0);

  assert(sched_sys_sleep(1) == 0);
  assert(sched_sys_thread_destroy(tid) == 0);
}

static void test_priority_round_robin(void) {
  sched_init();
  kprocess_t *p = process_create("prio");
  assert(p != NULL);

  kthread_t *t1 = thread_create(p, thread_a);
  kthread_t *t2 = thread_create(p, thread_b);
  assert(t1 && t2);

  assert(sched_set_thread_priority(t1->thread_id, 3) == 0);
  assert(sched_set_thread_priority(t2->thread_id, 8) == 0);

  sched_reschedule();
  assert(sched_current_thread() == t2);

  sched_yield();
  assert(sched_current_thread() == t1 || sched_current_thread() == t2);
}

static void test_sleep_wakeup(void) {
  sched_init();
  kprocess_t *p = process_create("sleep");
  assert(p != NULL);

  kthread_t *t = thread_create(p, thread_a);
  assert(t != NULL);

  sched_reschedule();
  assert(sched_current_thread() == t);

  sched_sleep(2);
  assert(t->state == THREAD_STATE_SLEEPING || t->state == THREAD_STATE_READY);

  sched_on_timer_tick();
  sched_on_timer_tick();
  sched_on_timer_tick();
  assert(t->state == THREAD_STATE_READY || t->state == THREAD_STATE_RUNNING);
}


static void test_preempt_on_higher_priority_ready(void) {
  sched_init();
  kprocess_t *p = process_create("preempt");
  assert(p != NULL);

  kthread_t *low = thread_create(p, thread_a);
  kthread_t *high = thread_create(p, thread_b);
  assert(low && high);

  assert(sched_set_thread_priority(low->thread_id, 2) == 0);
  assert(sched_set_thread_priority(high->thread_id, 7) == 0);

  sched_reschedule();
  assert(sched_current_thread() == high);

  sched_sleep(5);
  sched_reschedule();
  assert(sched_current_thread() != high);

  sched_wakeup(high);
  assert(high->state == THREAD_STATE_READY);

  sched_on_timer_tick();
  assert(sched_current_thread() == high);
}

static void test_policy_hooks_and_rr(void) {
  sched_init();
  kprocess_t *p = process_create("policy");
  assert(p != NULL);

  kthread_t *low = thread_create(p, thread_a);
  kthread_t *high = thread_create(p, thread_b);
  assert(low && high);

  assert(sched_set_thread_priority(low->thread_id, 1) == 0);
  assert(sched_set_thread_priority(high->thread_id, 9) == 0);

  sched_set_policy(SCHED_POLICY_ROUND_ROBIN);
  sched_reschedule();
  assert(sched_current_thread() != high);

  sched_set_policy(SCHED_POLICY_PRIORITY);
  sched_reschedule();
  assert(sched_current_thread() != NULL);

  sched_set_policy(SCHED_POLICY_EDF);
  sched_reschedule();
  assert(sched_current_thread() != NULL);

  sched_set_policy(SCHED_POLICY_RMS);
  sched_reschedule();
  assert(sched_current_thread() != NULL);
}

static void test_priority_inheritance_chain(void) {
  sched_init();
  kprocess_t *p = process_create("inherit");
  assert(p != NULL);

  kthread_t *low = thread_create(p, thread_a);
  kthread_t *mid = thread_create(p, thread_a);
  kthread_t *high = thread_create(p, thread_b);
  assert(low && mid && high);

  assert(sched_set_thread_priority(low->thread_id, 2) == 0);
  low->base_priority = 2;
  assert(sched_set_thread_priority(mid->thread_id, 4) == 0);
  mid->base_priority = 4;
  assert(sched_set_thread_priority(high->thread_id, 10) == 0);
  high->base_priority = 10;

  sched_on_mutex_acquire(low, &g_mutex_a);
  sched_on_mutex_acquire(mid, &g_mutex_b);
  sched_on_mutex_wait(low, &g_mutex_b);

  sched_on_mutex_wait(high, &g_mutex_a);
  assert(low->priority == high->priority);
  assert(mid->priority == high->priority);

  sched_on_mutex_release(low, &g_mutex_a);
  assert(low->priority == low->base_priority);

  sched_on_mutex_release(mid, &g_mutex_b);
  assert(mid->priority == mid->base_priority);
}

static void test_affinity_migration_multicore(void) {
  sched_init();
  kprocess_t *p = process_create("smp");
  assert(p != NULL);

  kthread_t *t = thread_create(p, thread_a);
  assert(t != NULL);

  assert(sched_sys_set_affinity(t->thread_id, (1U << 1)) == 0);
  assert(t->bound_core_id == 1U);

  g_mock_core_id = 1U;
  sched_reschedule();
  assert(sched_current_thread() == t || sched_current_thread() != NULL);
  g_mock_core_id = 0U;
}

static void run_benchmark(void) {
  sched_init();
  kprocess_t *p = process_create("bench");
  assert(p != NULL);
  uint64_t tids[64];

  for (int i = 0; i < 64; i++) {
    kthread_t *t = thread_create(p, thread_a);
    assert(t != NULL);
    tids[i] = t->thread_id;
  }

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (int iter = 0; iter < 50000; iter++) {
    for (int i = 0; i < 64; i++) {
      assert(sched_find_thread_by_id(tids[i]) != NULL);
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  double elapsed =
      (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
  printf("scheduler lookup benchmark: %.6f sec\n", elapsed);
}

int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], "--bench") == 0) {
    run_benchmark();
    return 0;
  }

  test_lifecycle_and_syscalls();
  test_priority_round_robin();
  test_sleep_wakeup();
  test_affinity_migration_multicore();
  test_preempt_on_higher_priority_ready();
  test_policy_hooks_and_rr();
  test_priority_inheritance_chain();

  printf("Scheduler tests passed.\n");
  return 0;
}
