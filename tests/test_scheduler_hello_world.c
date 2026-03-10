#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/ipc_async.h"
#include "../kernel/include/sched.h"

static uint32_t g_mock_core_id = 0;
static address_space_t g_as = {.root_table = 0x2000U};

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

static void hello_a(void) {}
static void hello_b(void) {}

int main(void) {
  printf("hello scheduler test app\n");

  sched_init();
  kprocess_t *p = process_create("hello");
  assert(p != NULL);

  kthread_t *a = thread_create(p, hello_a);
  kthread_t *b = thread_create(p, hello_b);
  assert(a && b);

  assert(sched_set_thread_priority(a->thread_id, 3) == 0);
  assert(sched_set_thread_priority(b->thread_id, 9) == 0);

  sched_reschedule();
  assert(sched_current_thread() == b);

  sched_sleep(2);
  sched_reschedule();
  assert(sched_current_thread() != b);

  sched_on_timer_tick();
  sched_on_timer_tick();
  sched_on_timer_tick();
  assert(b->state == THREAD_STATE_READY || b->state == THREAD_STATE_RUNNING);

  printf("hello scheduler test app passed\n");
  return 0;
}
