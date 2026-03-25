#include "sched/sched.h"
#include <bharat/cpu_local.h>
#include "sched/sched_deg.h"

#include "sched/algo_matrix.h"
#include "../../staging/formal/formal_verif.h"
#include "capability.h"
#include "core/multikernel.h"
#include "hal/hal.h"
#include "kernel_safety.h"
#include "list.h"
#include "panic.h"
#include "arch/context_switch.h"
#include "arch/arch_ext_state.h"
#include "arch/arch_cpu_caps.h"
#include "slab.h"
#include "ipc_async.h"
#include "mm/mm_aspace_switch.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SCHED_MAX_THREADS 128U
#define SCHED_MAX_PROCESSES 32U
#define SCHED_DEFAULT_SLICE_MS 10U

#define SCHED_MAX_PENDING_SUGGESTIONS 64U
#define SCHED_AFFINITY_ANY 0xFFFFFFFFU

#define CFS_NICE_0_WEIGHT 1024U

typedef struct {
  uint8_t in_use;
  uint8_t is_bootstrap;
  uint32_t next_free;
  kthread_t thread;
  cpu_context_t context;
  ai_sched_context_t ai_ctx;
  list_head_t run_node;
  list_head_t wait_node; // Used for both sleeping_list and blocked_list
  uint32_t reap_next;
  uint8_t reap_pending;
  uint8_t is_on_runqueue;
  uint8_t is_sleeping;
  uint8_t is_blocked;
} thread_slot_t;

typedef struct {
  uint8_t in_use;
  uint32_t next_free;
  kprocess_t process;
} process_slot_t;

typedef struct {
  ai_suggestion_t queue[SCHED_MAX_PENDING_SUGGESTIONS];
  uint32_t head;
  uint32_t tail;
} suggestion_queue_t;

typedef struct {
  void *mutex;
  kthread_t *owner;
} mutex_owner_entry_t;

// Removed core_runqueue_t definition from here as it's now in sched.h as sched_rq_t
// Removed static core_runqueue_t g_runqueues
static thread_slot_t g_threads[SCHED_MAX_THREADS];
static thread_slot_t g_bootstrap_threads[MAX_SUPPORTED_CORES][2];
static process_slot_t g_processes[SCHED_MAX_PROCESSES];
static uint8_t g_bootstrap_stacks[MAX_SUPPORTED_CORES][2][16384U];

static sched_policy_t g_policy = SCHED_POLICY_PRIORITY;
static uint64_t g_next_thread_id = 1U;
static uint64_t g_next_process_id = 1U;
static uint64_t g_sched_ticks = 0U;
static uint64_t g_sched_context_switches = 0U;
static suggestion_queue_t g_pending_suggestions;

static mutex_owner_entry_t g_mutex_owners[SCHED_MAX_THREADS];

static uint32_t g_free_thread_head = UINT32_MAX;
static uint32_t g_free_process_head = UINT32_MAX;
static uint32_t g_reap_head = UINT32_MAX;
static uint32_t g_reap_tail = UINT32_MAX;
static spinlock_t g_reap_lock;
static uint8_t g_sched_initialized = 0U;
static uint32_t g_active_core_count = 1U;
#if defined(TESTING)
static uint32_t g_sched_test_core_count = 1U;
#endif

enum {
  SCHED_BOOTSTRAP_IDLE = 0,
  SCHED_BOOTSTRAP_MONITOR = 1,
  SCHED_BOOTSTRAP_THREAD_TYPES = 2
};

void fv_secure_context_switch(void *next_thread_frame) __attribute__((weak));
static inline void sched_ready_bitmap_set(sched_rq_t *rq, uint32_t prio);
static inline void sched_ready_bitmap_clear_if_empty(sched_rq_t *rq, uint32_t prio);
static int sched_pick_priority_from_bitmap(const sched_rq_t *rq, int highest);

// CFS Functions
static void sched_cfs_enqueue(sched_rq_t *rq, kthread_t *thread);
static void sched_cfs_dequeue(sched_rq_t *rq, kthread_t *thread);
static kthread_t *sched_cfs_pick_next(sched_rq_t *rq);
static void sched_cfs_update_vruntime(sched_rq_t *rq, kthread_t *thread, uint64_t delta_exec);
static void sched_validate_rq(sched_rq_t *rq);

static uint32_t sched_clamp_core(uint32_t core_id) {
  if (core_id >= g_active_core_count) {
    return 0U;
  }
  return core_id;
}

static uint32_t sched_configured_core_count(void) {
#if defined(TESTING)
  uint32_t test_cores = g_sched_test_core_count;
  if (test_cores == 0U) {
    test_cores = 1U;
  }
  if (test_cores > MAX_SUPPORTED_CORES) {
    test_cores = MAX_SUPPORTED_CORES;
  }
  return test_cores;
#else
  return MAX_SUPPORTED_CORES;
#endif
}

static thread_slot_t *sched_find_bootstrap_slot_by_tid(uint64_t tid) {
  for (uint32_t core = 0; core < g_active_core_count; ++core) {
    for (uint32_t i = 0; i < SCHED_BOOTSTRAP_THREAD_TYPES; ++i) {
      thread_slot_t *slot = &g_bootstrap_threads[core][i];
      if (slot->in_use != 0U && slot->thread.thread_id == tid) {
        return slot;
      }
    }
  }
  return NULL;
}

void arch_post_switch(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  hal_cpu_enable_interrupts();
}

static thread_slot_t *sched_find_thread_slot_by_tid(uint64_t tid) {
  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
    if (g_threads[i].in_use != 0U && g_threads[i].thread.thread_id == tid) {
      return &g_threads[i];
    }
  }
  return sched_find_bootstrap_slot_by_tid(tid);
}

static thread_slot_t *sched_find_free_thread_slot(void) {
  if (g_free_thread_head == UINT32_MAX) {
    return NULL;
  }
  uint32_t idx = g_free_thread_head;
  g_free_thread_head = g_threads[idx].next_free;
  return &g_threads[idx];
}

static process_slot_t *sched_find_free_process_slot(void) {
  if (g_free_process_head == UINT32_MAX) {
    return NULL;
  }
  uint32_t idx = g_free_process_head;
  g_free_process_head = g_processes[idx].next_free;
  return &g_processes[idx];
}

static void sched_sleep_enqueue(thread_slot_t *slot, uint32_t core_id) {
  if (!slot || slot->is_sleeping != 0U || slot->is_blocked != 0U) {
    return;
  }
  list_add(&slot->wait_node, &g_cpu_locals[core_id].runqueue.sleeping_list);
  slot->is_sleeping = 1U;
}

static void sched_sleep_dequeue(thread_slot_t *slot) {
  if (!slot || slot->is_sleeping == 0U) {
    return;
  }
  list_del(&slot->wait_node);
  list_init(&slot->wait_node);
  slot->is_sleeping = 0U;
}

static void sched_block_enqueue(thread_slot_t *slot, uint32_t core_id) {
  if (!slot || slot->is_sleeping != 0U || slot->is_blocked != 0U) {
    return;
  }
  list_add(&slot->wait_node, &g_cpu_locals[core_id].runqueue.blocked_list);
  slot->is_blocked = 1U;
}

static void sched_block_dequeue(thread_slot_t *slot) {
  if (!slot || slot->is_blocked == 0U) {
    return;
  }
  list_del(&slot->wait_node);
  list_init(&slot->wait_node);
  slot->is_blocked = 0U;
}

static void sched_detach_thread_from_queues(thread_slot_t *slot) {
  if (!slot) {
    return;
  }
  kthread_t *thread = &slot->thread;
  uint32_t core_id = sched_clamp_core(thread->bound_core_id);
  sched_rq_t *rq = &g_cpu_locals[core_id].runqueue;

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  bool is_local = (core_id == current_core);

  if (is_local) {
      hal_cpu_disable_interrupts();
  } else {
      spin_lock(&rq->lock);
  }

  if (slot->is_on_runqueue != 0U) {
    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
      sched_cfs_dequeue(rq, thread);
    } else {
      list_del(&slot->run_node);
      list_init(&slot->run_node);
      sched_ready_bitmap_clear_if_empty(rq, thread->priority);
    }
    slot->is_on_runqueue = 0U;
    if (rq->runnable_count > 0U) {
      rq->runnable_count--;
    }
    sched_validate_rq(rq);
  }
  if (slot->is_sleeping != 0U) {
    sched_sleep_dequeue(slot);
  }
  if (slot->is_blocked != 0U) {
    sched_block_dequeue(slot);
  }

  if (is_local) {
      hal_cpu_enable_interrupts();
  } else {
      spin_unlock(&rq->lock);
  }
}

static int sched_enqueue_reap(thread_slot_t *slot) {
  if (!slot) {
    return -1;
  }
  if (slot->is_bootstrap != 0U) {
    return -1;
  }

  spin_lock(&g_reap_lock);
  if (slot->reap_pending != 0U) {
    spin_unlock(&g_reap_lock);
    return 0;
  }

  uint32_t idx = (uint32_t)(slot - g_threads);
  slot->reap_pending = 1U;
  slot->reap_next = UINT32_MAX;
  if (g_reap_tail == UINT32_MAX) {
    g_reap_head = idx;
    g_reap_tail = idx;
  } else {
    g_threads[g_reap_tail].reap_next = idx;
    g_reap_tail = idx;
  }
  spin_unlock(&g_reap_lock);
  return 0;
}

static int sched_mark_thread_terminated(kthread_t *thread) {
  if (!thread) {
    return -1;
  }
  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
  if (!slot) {
    return -1;
  }
  if (thread->state == THREAD_STATE_TERMINATED) {
    return sched_enqueue_reap(slot);
  }

  thread->state = THREAD_STATE_TERMINATED;
  if (thread != sched_current_thread()) {
    sched_detach_thread_from_queues(slot);
  }
  return sched_enqueue_reap(slot);
}

static void sched_reap_terminated_threads(void) {
  while (1) {
    thread_slot_t *slot = NULL;

    spin_lock(&g_reap_lock);
    if (g_reap_head != UINT32_MAX) {
      uint32_t idx = g_reap_head;
      slot = &g_threads[idx];
      g_reap_head = slot->reap_next;
      if (g_reap_head == UINT32_MAX) {
        g_reap_tail = UINT32_MAX;
      }
      slot->reap_next = UINT32_MAX;
      slot->reap_pending = 0U;
    }
    spin_unlock(&g_reap_lock);

    if (!slot) {
      break;
    }
    (void)thread_destroy(&slot->thread);
  }
}

int sched_enqueue(kthread_t *thread, uint32_t core_id) {
  if (!thread || thread->priority >= MAX_PRIORITY_LEVELS) {
    return -1;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
  if (!slot) {
    return -1;
  }

  core_id = sched_clamp_core(core_id);
  sched_rq_t *rq = &g_cpu_locals[core_id].runqueue;

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  bool is_local = (core_id == current_core);

  if (!is_local) {
      spin_lock(&rq->lock);
      thread->bound_core_id = core_id;
      list_add(&slot->wait_node, &rq->pending_inbox);
      rq->remote_enqueues++;

      bool send_ipi = false;
      if (rq->resched_pending == 0U) {
          rq->resched_pending = 1U;
          send_ipi = true;
          rq->ipi_sent++;
      } else {
          rq->ipi_coalesced++;
      }
      spin_unlock(&rq->lock);

      if (send_ipi) {
          hal_send_ipi_payload(core_id, 0); // Payload 0 or specific URPC msg if we use URPC
      }
      return 0;
  }

  hal_cpu_disable_interrupts();

  if (slot->is_on_runqueue != 0U) {
    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
      sched_cfs_dequeue(rq, thread);
    } else {
      list_del(&slot->run_node);
      list_init(&slot->run_node);
      sched_ready_bitmap_clear_if_empty(rq, thread->priority);
    }
    slot->is_on_runqueue = 0U;
    if (rq->runnable_count > 0U) {
      rq->runnable_count--;
    }
  }

  thread->bound_core_id = core_id;
  thread->state = THREAD_STATE_READY;

  if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
    sched_cfs_enqueue(rq, thread);
  } else {
    list_add(&slot->run_node, &rq->ready_queue[thread->priority]);
    sched_ready_bitmap_set(rq, thread->priority);
  }

  slot->is_on_runqueue = 1U;
  rq->runnable_count++;

  sched_validate_rq(rq);

  hal_cpu_enable_interrupts();
  return 0;
}

static void sched_idle_task(void) {
  while (1) {
    hal_cpu_halt();
  }
}

static void sched_monitor_task(void) {
  uint32_t core_id = hal_cpu_get_id();
  uint64_t last_check = 0;
  while (1) {
    uint64_t now = sched_get_ticks();
    if (now - last_check >= 1000) {
      last_check = now;
      // Monitor logic placeholder
      if (core_id == 0) {
        // BSP monitor might do system-wide coordination
      }
    }
    kthread_yield();
  }
}

void sched_thread_exit_trampoline(void) {
  kthread_t *current = sched_current_thread();
  if (current) {
    uint32_t core = sched_clamp_core(hal_cpu_get_id());
    (void)sched_mark_thread_terminated(current);
    g_cpu_locals[core].runqueue.current_thread = NULL;
    sched_reschedule();
  }
  while (1) {
    hal_cpu_halt();
  }
}

static void sched_reset_core_runqueues(void) {
  for (uint32_t core = 0; core < g_active_core_count; ++core) {
    sched_rq_t *rq = &g_cpu_locals[core].runqueue;
    rq->current_thread = NULL;
    rq->idle_thread = NULL;
    rq->total_ticks = 0U;
    rq->context_switches = 0U;
    rq->runnable_count = 0U;
    rq->throttled = 0U;
    rq->resched_pending = 0U;
    rq->remote_enqueues = 0U;
    rq->ipi_sent = 0U;
    rq->ipi_coalesced = 0U;
    rq->inbox_drains = 0U;
    rq->remote_preemptions = 0U;
    spin_lock_init(&rq->lock);
    list_init(&rq->sleeping_list);
    list_init(&rq->blocked_list);
    list_init(&rq->pending_inbox);
    for (uint32_t p = 0; p < MAX_PRIORITY_LEVELS; ++p) {
      list_init(&rq->ready_queue[p]);
    }
    rq->ready_bitmap = 0U;
  }
}

static kthread_t *sched_create_bootstrap_thread(kprocess_t *parent,
                                                uint32_t core,
                                                uint32_t kind,
                                                void (*entry_point)(void),
                                                uint32_t priority,
                                                uint8_t enqueue) {
  thread_slot_t *slot = &g_bootstrap_threads[core][kind];
  memset(slot, 0, sizeof(*slot));
  slot->in_use = 1U;
  slot->is_bootstrap = 1U;

  slot->thread.thread_id = g_next_thread_id++;
  slot->thread.process_id = parent ? parent->process_id : 0U;
  slot->thread.process = parent;
  slot->thread.personality = PERSONALITY_NATIVE;
  slot->thread.state = THREAD_STATE_READY;
  slot->thread.priority = priority;
  slot->thread.base_priority = priority;
  slot->thread.time_slice_ms = SCHED_DEFAULT_SLICE_MS;
  slot->thread.bound_core_id = core;
  slot->thread.affinity_mask = (1U << core);
  slot->thread.cpu_context = &slot->context;
  slot->thread.kernel_stack = (virt_addr_t)(uintptr_t)&g_bootstrap_stacks[core][kind][0];

  arch_prepare_initial_context(
      &slot->context, entry_point,
      (uint64_t)(uintptr_t)&g_bootstrap_stacks[core][kind][0] +
          (uint64_t)sizeof(g_bootstrap_stacks[core][kind]));

  ai_sched_init_context(&slot->ai_ctx);
  slot->ai_ctx.thread_id = (uint32_t)slot->thread.thread_id;
  slot->thread.ai_sched_ctx = &slot->ai_ctx;
  list_init(&slot->run_node);
  list_init(&slot->wait_node);

  if (enqueue != 0U) {
    (void)sched_enqueue(&slot->thread, core);
  }
  return &slot->thread;
}

#if defined(TESTING)
void sched_set_test_core_count(uint32_t core_count) {
  if (core_count == 0U) {
    g_sched_test_core_count = 1U;
  } else if (core_count > MAX_SUPPORTED_CORES) {
    g_sched_test_core_count = MAX_SUPPORTED_CORES;
  } else {
    g_sched_test_core_count = core_count;
  }
}

void sched_test_reset(void) {
  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
    if (g_threads[i].in_use != 0U) {
      if (g_threads[i].thread.capability_list) {
        cap_table_destroy(g_threads[i].thread.capability_list);
        g_threads[i].thread.capability_list = NULL;
      }
      arch_ext_state_thread_destroy(&g_threads[i].thread);
      if (g_threads[i].thread.kernel_stack) {
        kfree((void *)(uintptr_t)g_threads[i].thread.kernel_stack);
      }
    }
  }
  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_processes); ++i) {
    if (g_processes[i].in_use != 0U) {
      if (g_processes[i].process.security_sandbox_ctx) {
        cap_table_destroy(g_processes[i].process.security_sandbox_ctx);
        g_processes[i].process.security_sandbox_ctx = NULL;
      }
      if (g_processes[i].process.addr_space) {
        (void)aspace_destroy(g_processes[i].process.addr_space);
      }
    }
  }
  memset(g_bootstrap_threads, 0, sizeof(g_bootstrap_threads));
  sched_reset_core_runqueues();
  g_sched_initialized = 0U;
}
#endif

void sched_init(void) {
  if (g_sched_initialized != 0U) {
#if defined(TESTING)
    sched_test_reset();
#else
    return;
#endif
  }

  g_active_core_count = sched_configured_core_count();
  g_free_thread_head = 0U;
  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
    g_threads[i].in_use = 0U;
    g_threads[i].is_bootstrap = 0U;
    g_threads[i].next_free = (i + 1U < BHARAT_ARRAY_SIZE(g_threads)) ? (uint32_t)(i + 1U) : UINT32_MAX;
    g_threads[i].reap_next = UINT32_MAX;
    g_threads[i].reap_pending = 0U;
  }

  g_free_process_head = 0U;
  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_processes); ++i) {
    g_processes[i].in_use = 0U;
    g_processes[i].next_free = (i + 1U < BHARAT_ARRAY_SIZE(g_processes)) ? (uint32_t)(i + 1U) : UINT32_MAX;
  }

  g_next_thread_id = 1U;
  g_next_process_id = 1U;
  g_sched_ticks = 0U;
  g_sched_context_switches = 0U;
  g_pending_suggestions.head = 0U;
  g_pending_suggestions.tail = 0U;
  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_mutex_owners); ++i) {
    g_mutex_owners[i].mutex = NULL;
    g_mutex_owners[i].owner = NULL;
  }
  g_reap_head = UINT32_MAX;
  g_reap_tail = UINT32_MAX;
  spin_lock_init(&g_reap_lock);
  memset(g_bootstrap_threads, 0, sizeof(g_bootstrap_threads));
  sched_reset_core_runqueues();

  kprocess_t *idle_process = process_create("idle_process");
  for (uint32_t core = 0; core < g_active_core_count; ++core) {
    kthread_t *idle = sched_create_bootstrap_thread(
        idle_process, core, SCHED_BOOTSTRAP_IDLE, sched_idle_task, 0U, 0U);
    g_cpu_locals[core].runqueue.idle_thread = idle;
    g_cpu_locals[core].runqueue.current_thread = idle;
#if !defined(TESTING)
    (void)sched_create_bootstrap_thread(idle_process, core, SCHED_BOOTSTRAP_MONITOR,
                                        sched_monitor_task, SCHED_MAX_PRIORITY, 1U);
#endif
  }
  g_sched_initialized = 1U;
}

kprocess_t *process_create(const char *name) {
  (void)name;
  process_slot_t *slot = sched_find_free_process_slot();
  if (!slot) {
    return NULL;
  }

  slot->in_use = 1U;
  slot->process.process_id = g_next_process_id++;
  slot->process.addr_space = mm_create_address_space();
  slot->process.main_thread = NULL;
  slot->process.security_sandbox_ctx = NULL;

  // Explicit multikernel ownership metadata
  slot->process.owner_core_id = hal_cpu_get_id();
  slot->process.object_id = slot->process.process_id;

  if (!slot->process.addr_space || cap_table_init_for_process(&slot->process) != 0) {
    slot->in_use = 0U;
    uint32_t idx = (uint32_t)(slot - g_processes);
    slot->next_free = g_free_process_head;
    g_free_process_head = idx;
    return NULL;
  }

  return &slot->process;
}

int process_destroy(kprocess_t *process) {
  if (!process) {
    return -1;
  }

  process_slot_t *slot = NULL;
  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_processes); ++i) {
    if (g_processes[i].in_use != 0U && &g_processes[i].process == process) {
      slot = &g_processes[i];
      break;
    }
  }

  if (!slot) {
    return -1;
  }

  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
    if (g_threads[i].in_use != 0U &&
        g_threads[i].thread.process_id == slot->process.process_id) {
      return -1;
    }
  }

  if (slot->process.addr_space) {
    (void)aspace_destroy(slot->process.addr_space);
    slot->process.addr_space = NULL;
  }

  if (slot->process.security_sandbox_ctx) {
    cap_table_destroy(slot->process.security_sandbox_ctx);
    slot->process.security_sandbox_ctx = NULL;
  }

  slot->in_use = 0U;
  uint32_t idx = (uint32_t)(slot - g_processes);
  slot->next_free = g_free_process_head;
  g_free_process_head = idx;
  return 0;
}

kthread_t *thread_create_detached(kprocess_t *parent, void (*entry_point)(void)) {
  thread_slot_t *slot = sched_find_free_thread_slot();
  if (!slot) {
    return NULL;
  }

  slot->in_use = 1U;
  slot->is_bootstrap = 0U;
  slot->is_on_runqueue = 0U;
  slot->is_sleeping = 0U;
  slot->is_blocked = 0U;

  slot->thread.thread_id = g_next_thread_id++;
  slot->thread.process_id = parent ? parent->process_id : 0U;
  slot->thread.process = parent;
  slot->thread.personality = PERSONALITY_NATIVE;
  slot->thread.state = THREAD_STATE_READY;
  slot->thread.priority = 1U;
  slot->thread.base_priority = 1U;
  slot->thread.cpu_time_consumed = 0U;
  slot->thread.time_slice_ms = SCHED_DEFAULT_SLICE_MS;
  slot->thread.preferred_numa_node = 0U;
  slot->thread.bound_core_id = sched_clamp_core(hal_cpu_get_id());
  slot->thread.affinity_mask = SCHED_AFFINITY_ANY;
  slot->thread.wake_deadline_ms = 0U;
  slot->thread.context_switch_count = 0U;

  #define KERNEL_STACK_SIZE 16384U
  void *stack = kmalloc(KERNEL_STACK_SIZE);
  if (!stack) {
    slot->in_use = 0U;
    uint32_t idx = (uint32_t)(slot - g_threads);
    slot->next_free = g_free_thread_head;
    g_free_thread_head = idx;
    return NULL;
  }

  slot->thread.kernel_stack = (virt_addr_t)(uintptr_t)stack;
  uint64_t stack_top = (uint64_t)(uintptr_t)stack + KERNEL_STACK_SIZE;

  slot->thread.cpu_context = &slot->context;
  arch_prepare_initial_context(&slot->context, entry_point, stack_top);

  // Initialize architecture-specific extended CPU state (e.g. FPU/Vector)
  if (arch_ext_state_thread_init(&slot->thread) != 0) {
    kfree(stack);
    slot->in_use = 0U;
    uint32_t idx = (uint32_t)(slot - g_threads);
    slot->next_free = g_free_thread_head;
    g_free_thread_head = idx;
    return NULL;
  }

  ai_sched_init_context(&slot->ai_ctx);
  slot->ai_ctx.thread_id = (uint32_t)slot->thread.thread_id;
  slot->thread.ai_sched_ctx = &slot->ai_ctx;

  list_init(&slot->run_node);
  list_init(&slot->wait_node);

  if (parent && !parent->main_thread) {
    parent->main_thread = &slot->thread;
  }

  return &slot->thread;
}

kthread_t *thread_create(kprocess_t *parent, void (*entry_point)(void)) {
  kthread_t *thread = thread_create_detached(parent, entry_point);
  if (thread && entry_point != (void (*)(void))sched_idle_task) {
    (void)sched_enqueue(thread, thread->bound_core_id);
  }
  return thread;
}

int thread_destroy(kthread_t *thread) {
  // Reaper-only contract:
  // - call only from deferred reap context, never inline on the running thread.
  // - never destroy while executing on the victim thread's stack.
  if (!thread) {
    return -1;
  }
  if (thread == sched_current_thread()) {
    return -1;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
  if (!slot) {
    return -1;
  }

  sched_detach_thread_from_queues(slot);

  // Clean up architecture extended state
  arch_ext_state_thread_destroy(thread);

  if (thread->capability_list) {
    cap_table_destroy(thread->capability_list);
    thread->capability_list = NULL;
  }

  if (thread->kernel_stack) {
    kfree((void*)(uintptr_t)thread->kernel_stack);
    thread->kernel_stack = 0U;
  }

  slot->in_use = 0U;
  uint32_t idx = (uint32_t)(slot - g_threads);
  slot->next_free = g_free_thread_head;
  g_free_thread_head = idx;
  return 0;
}

static kthread_t *sched_find_mutex_owner(void *mutex) {
  if (!mutex) {
    return NULL;
  }

  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_mutex_owners); ++i) {
    if (g_mutex_owners[i].mutex == mutex) {
      return g_mutex_owners[i].owner;
    }
  }

  return NULL;
}

static void sched_register_mutex_owner(void *mutex, kthread_t *owner) {
  if (!mutex) {
    return;
  }

  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_mutex_owners); ++i) {
    if (g_mutex_owners[i].mutex == mutex || g_mutex_owners[i].mutex == NULL) {
      g_mutex_owners[i].mutex = mutex;
      g_mutex_owners[i].owner = owner;
      return;
    }
  }
}

static void sched_unregister_mutex_owner(void *mutex, kthread_t *owner) {
  if (!mutex) {
    return;
  }

  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_mutex_owners); ++i) {
    if (g_mutex_owners[i].mutex == mutex &&
        (owner == NULL || g_mutex_owners[i].owner == owner)) {
      g_mutex_owners[i].mutex = NULL;
      g_mutex_owners[i].owner = NULL;
      return;
    }
  }
}

static int sched_suggestion_dequeue(ai_suggestion_t *out) {
  if (!out || g_pending_suggestions.head == g_pending_suggestions.tail) {
    return -1;
  }
  *out = g_pending_suggestions.queue[g_pending_suggestions.tail];
  g_pending_suggestions.tail = (g_pending_suggestions.tail + 1U) % SCHED_MAX_PENDING_SUGGESTIONS;
  return 0;
}

static void sched_process_pending_ai_suggestions(void) {
  ai_suggestion_t suggestion = {0};
  while (sched_suggestion_dequeue(&suggestion) == 0) {
    (void)sched_ai_apply_suggestion(&suggestion);
  }
}

static uint32_t sched_run_queue_depth(uint32_t core_id) {
  return g_cpu_locals[core_id].runqueue.runnable_count;
}

static inline void sched_ready_bitmap_set(sched_rq_t *rq, uint32_t prio) {
  if (!rq || prio >= MAX_PRIORITY_LEVELS) {
    return;
  }
  rq->ready_bitmap |= (1U << prio);
}

static inline void sched_ready_bitmap_clear_if_empty(sched_rq_t *rq, uint32_t prio) {
  if (!rq || prio >= MAX_PRIORITY_LEVELS) {
    return;
  }
  if (list_empty(&rq->ready_queue[prio])) {
    rq->ready_bitmap &= ~(1U << prio);
  }
}

static int sched_pick_priority_from_bitmap(const sched_rq_t *rq, int highest) {
  if (!rq || rq->ready_bitmap == 0U) {
    return -1;
  }
  if (highest != 0) {
    return 31 - __builtin_clz(rq->ready_bitmap);
  }
  return __builtin_ctz(rq->ready_bitmap);
}

static void sched_update_telemetry(kthread_t *thread) {
  if (!thread || !thread->ai_sched_ctx) {
    return;
  }
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  ai_sched_collect_sample(thread->ai_sched_ctx, thread->time_slice_ms,
                          thread->cpu_time_consumed,
                          sched_run_queue_depth(core),
                          (uint32_t)thread->context_switch_count);
}

static kthread_t *sched_pick_next_ready(uint32_t core_id) {
  core_id = sched_clamp_core(core_id);
  sched_rq_t *rq = &g_cpu_locals[core_id].runqueue;
  int pick_highest = (g_policy == SCHED_POLICY_ROUND_ROBIN) ? 0 : 1;
  int prio = sched_pick_priority_from_bitmap(rq, pick_highest);
  if (prio < 0) {
    return rq->idle_thread;
  }

  list_head_t *head = &rq->ready_queue[prio];
  list_head_t *node = head->prev;
  thread_slot_t *slot = (thread_slot_t *)(void *)((char *)node - offsetof(thread_slot_t, run_node));
  list_del(node);
  list_init(node);
  slot->is_on_runqueue = 0U;
  if (rq->runnable_count > 0U) {
    rq->runnable_count--;
  }
  sched_ready_bitmap_clear_if_empty(rq, (uint32_t)prio);
  return &slot->thread;
}

static void sched_dequeue_task_l0(kthread_t *thread, uint32_t core_id) {
  (void)core_id;
  if (!thread) {
    return;
  }
  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
  if (slot && slot->is_on_runqueue != 0U) {
    sched_rq_t *rq = &g_cpu_locals[core_id].runqueue;
    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
      sched_cfs_dequeue(rq, thread);
    } else {
      list_del(&slot->run_node);
      list_init(&slot->run_node);
      sched_ready_bitmap_clear_if_empty(rq, thread->priority);
    }
    slot->is_on_runqueue = 0U;
    if (rq->runnable_count > 0U) {
      rq->runnable_count--;
    }
  }
}

void sched_enqueue_task_l0(kthread_t *thread, uint32_t core_id) {
  (void)sched_enqueue(thread, core_id);
}

kthread_t *sched_pick_next_ready_l0(uint32_t core_id) {
  return sched_pick_next_ready(core_id);
}

void sched_enqueue_task_l1(kthread_t *thread, uint32_t core_id) {
  (void)sched_enqueue(thread, core_id);
}

void sched_dequeue_task_l1(kthread_t *thread, uint32_t core_id) {
  sched_dequeue_task_l0(thread, core_id);
}

kthread_t *sched_pick_next_ready_l1(uint32_t core_id) {
  return sched_pick_next_ready(core_id);
}

static void sched_switch_to(kthread_t *next, uint32_t core_id) {
  if (!next) {
    hal_cpu_enable_interrupts();
    return;
  }

  kthread_t *current = g_cpu_locals[core_id].runqueue.current_thread;
  if (current == next) {
    hal_cpu_enable_interrupts();
    return;
  }

  sched_rq_t* rq = &g_cpu_locals[core_id].runqueue;

  if (current && current != rq->idle_thread &&
      current->state == THREAD_STATE_RUNNING) {

    thread_slot_t *slot = sched_find_thread_slot_by_tid(current->thread_id);
    if (slot && slot->is_on_runqueue == 0U) {
        current->state = THREAD_STATE_READY;
        if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
            sched_cfs_enqueue(rq, current);
        } else {
            list_add(&slot->run_node, &rq->ready_queue[current->priority]);
            sched_ready_bitmap_set(rq, current->priority);
        }
        slot->is_on_runqueue = 1U;
        rq->runnable_count++;
        sched_validate_rq(rq);
    }
  }

  next->state = THREAD_STATE_RUNNING;
  next->context_switch_count++;
  g_sched_context_switches++;
  g_cpu_locals[core_id].runqueue.context_switches++;
  g_cpu_locals[core_id].runqueue.current_thread = next;

  cpu_context_t *prev_ctx = current ? (cpu_context_t*)current->cpu_context : NULL;
  cpu_context_t *next_ctx = (cpu_context_t*)next->cpu_context;

  if (current) {
    arch_ext_state_save(current);
  }

  address_space_t *prev_as = current && current->process ? current->process->addr_space : NULL;
  address_space_t *next_as = next->process ? next->process->addr_space : NULL;
  mm_switch_active_aspace(core_id, prev_as, next_as);

  #ifndef NDEBUG
  vm_debug_validate_active_tracking();
  #endif

    // Process incoming URPC messages before doing the switch
    extern void vmm_process_local_urpc_messages(uint32_t core_id);
    vmm_process_local_urpc_messages(core_id);

  if (fv_secure_context_switch) {
    fv_secure_context_switch(next_ctx);
  } else {
        // local IRQs were disabled by hal_cpu_disable_interrupts() in sched_reschedule().
        // They will be explicitly re-enabled by arch_post_switch() which is
        // called from the assembly arch_context_switch once we are on the
        // next thread's stack.
    arch_context_switch(prev_ctx, next_ctx);
  }

  arch_ext_state_restore(next);
}

void sched_wait_queue_init(wait_queue_t* queue) {
  if (queue) {
    queue->head = NULL;
    queue->tail = NULL;
  }
}

void sched_wait_queue_enqueue(wait_queue_t* queue, kthread_t* thread) {
  if (!queue || !thread) {
    return;
  }

  thread->next_waiter = NULL;

  if (!queue->tail) {
    queue->head = thread;
    queue->tail = thread;
  } else {
    queue->tail->next_waiter = thread;
    queue->tail = thread;
  }
}

kthread_t* sched_wait_queue_dequeue(wait_queue_t* queue) {
  if (!queue || !queue->head) {
    return NULL;
  }

  kthread_t* thread = queue->head;
  // Skip any threads that were already woken up by timeout (state != BLOCKED)
  // or that had their next_waiter cleared by the timeout sweep.
  while (thread && thread->state != THREAD_STATE_BLOCKED) {
      thread = thread->next_waiter;
      queue->head = thread;
  }

  if (!queue->head) {
    queue->tail = NULL;
    return NULL;
  }

  thread = queue->head;
  queue->head = thread->next_waiter;
  if (!queue->head) {
    queue->tail = NULL;
  }

  thread->next_waiter = NULL;
  return thread;
}

void sched_block(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  kthread_t *current = g_cpu_locals[core].runqueue.current_thread;
  if (current) {
    current->state = THREAD_STATE_BLOCKED;
    if (current->sched_ctx && current->sched_ctx->deg) {
        deg_block_member(current, 0);
    }

    if (current->ipc_deadline_ticks > 0) {
      thread_slot_t *slot = sched_find_thread_slot_by_tid(current->thread_id);
      if (slot) {
        sched_block_enqueue(slot, core);
      }
    }

    g_cpu_locals[core].runqueue.current_thread = NULL;
  }
}

void sched_reschedule(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  sched_reap_terminated_threads();
  sched_process_pending_ai_suggestions();

  hal_cpu_disable_interrupts(); // Fast path local lockless

  sched_rq_t *rq = &g_cpu_locals[core].runqueue;

  // Empty remote enqueue inbox
  if (rq->resched_pending != 0U || !list_empty(&rq->pending_inbox)) {
      spin_lock(&rq->lock);
      rq->resched_pending = 0U; // Clear flag since we are draining now
      list_head_t *curr = rq->pending_inbox.next;
      uint32_t drained = 0;
      kthread_t *highest_prio_arrived = NULL;

      while (curr != &rq->pending_inbox) {
          thread_slot_t *slot = (thread_slot_t *)(void *)((char *)curr - offsetof(thread_slot_t, wait_node));
          curr = curr->next;

          list_del(&slot->wait_node);
          list_init(&slot->wait_node);

          kthread_t* thread = &slot->thread;
          thread->state = THREAD_STATE_READY;

          if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
            sched_cfs_enqueue(rq, thread);
          } else {
            list_add(&slot->run_node, &rq->ready_queue[thread->priority]);
            sched_ready_bitmap_set(rq, thread->priority);
          }

          if (!highest_prio_arrived || thread->priority > highest_prio_arrived->priority) {
              highest_prio_arrived = thread;
          }

          slot->is_on_runqueue = 1U;
          rq->runnable_count++;
          drained++;
      }
      if (drained > 0) {
          rq->inbox_drains++;
          if (rq->current_thread && highest_prio_arrived &&
              highest_prio_arrived->priority > rq->current_thread->priority) {
              rq->remote_preemptions++;
          }
      }
      spin_unlock(&rq->lock);
  }

  if (g_cpu_locals[core].runqueue.throttled != 0U && g_cpu_locals[core].runqueue.idle_thread) {
    sched_switch_to(g_cpu_locals[core].runqueue.idle_thread, core);
    return;
  }

  kthread_t *next = sched_pick_next_ready(core);
  sched_switch_to(next, core);
}

void kthread_yield(void) { sched_reschedule(); }

void sched_sleep(uint64_t millis) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  kthread_t *current = g_cpu_locals[core].runqueue.current_thread;
  if (!current || current == g_cpu_locals[core].runqueue.idle_thread) {
    return;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid(current->thread_id);
  if (!slot) {
    return;
  }

  current->wake_deadline_ms = g_sched_ticks + millis;
  current->state = THREAD_STATE_SLEEPING;
  sched_sleep_enqueue(slot, core);
  g_cpu_locals[core].runqueue.current_thread = NULL;
  sched_reschedule();
}

void sched_wakeup_with_priority(kthread_t *thread, uint32_t wakeup_priority) {
  if (!thread) {
    return;
  }

  if (wakeup_priority <= SCHED_MAX_PRIORITY && wakeup_priority > thread->priority) {
    thread->priority = wakeup_priority;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
  if (!slot) {
    return;
  }

  if (thread->state == THREAD_STATE_SLEEPING || thread->state == THREAD_STATE_BLOCKED) {
    thread->state = THREAD_STATE_READY;
    thread->wake_deadline_ms = 0U;
    if (slot->is_sleeping != 0U) {
      sched_sleep_dequeue(slot);
    }
    if (slot->is_blocked != 0U) {
      sched_block_dequeue(slot);
    }
    (void)sched_enqueue(thread, thread->bound_core_id);
  }
}

void sched_wakeup(kthread_t *thread) {
  sched_wakeup_with_priority(thread, SCHED_MAX_PRIORITY + 1U);
}

static void sched_balance_once(void) {
  uint32_t busiest = 0U;
  uint32_t idlest = 0U;
  uint32_t max_depth = 0U;
  uint32_t min_depth = UINT32_MAX;

  for (uint32_t core = 0; core < MAX_SUPPORTED_CORES; ++core) {
    uint32_t depth = sched_run_queue_depth(core);
    if (depth > max_depth) {
      max_depth = depth;
      busiest = core;
    }
    if (depth < min_depth) {
      min_depth = depth;
      idlest = core;
    }
  }

  if (max_depth <= (min_depth + 1U) || busiest == idlest) {
    return;
  }

  kthread_t *candidate = sched_pick_next_ready(busiest);
  if (!candidate || candidate == g_cpu_locals[busiest].runqueue.idle_thread) {
    return;
  }

  if ((candidate->affinity_mask & (1U << idlest)) == 0U) {
    (void)sched_enqueue(candidate, busiest);
    return;
  }

  candidate->preferred_numa_node = (uint8_t)idlest;
  (void)sched_enqueue(candidate, idlest);
}

void sched_on_timer_tick(void) {
  g_sched_ticks++;
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  g_cpu_locals[core].runqueue.total_ticks++;
  ipc_async_check_timeouts(g_sched_ticks);

  list_head_t *sleep_head = &g_cpu_locals[core].runqueue.sleeping_list;
  list_head_t *curr = sleep_head->next;
  while (curr != sleep_head) {
    thread_slot_t *slot = (thread_slot_t *)(void *)((char *)curr - offsetof(thread_slot_t, wait_node));
    curr = curr->next;
    if (slot->thread.state == THREAD_STATE_SLEEPING &&
        slot->thread.wake_deadline_ms <= g_sched_ticks) {
      sched_wakeup(&slot->thread);
    }
  }

  list_head_t *block_head = &g_cpu_locals[core].runqueue.blocked_list;
  curr = block_head->next;
  while (curr != block_head) {
    thread_slot_t *slot = (thread_slot_t *)(void *)((char *)curr - offsetof(thread_slot_t, wait_node));
    curr = curr->next;
    if (slot->thread.state == THREAD_STATE_BLOCKED &&
        slot->thread.ipc_deadline_ticks > 0 &&
        slot->thread.ipc_deadline_ticks <= g_sched_ticks) {
      slot->thread.ipc_wakeup_reason = -3; // IPC_ERR_WOULD_BLOCK or TIMEOUT
      slot->thread.ipc_deadline_ticks = 0;

      // Unlink it from wait queues handled by endpoint access so we can awaken it
      slot->thread.next_waiter = NULL;
      sched_wakeup(&slot->thread);
    }
  }

  sched_process_pending_ai_suggestions();
  sched_reap_terminated_threads();

  if ((g_sched_ticks % 16U) == 0U && core == 0U) {
    sched_balance_once();
  }

  sched_rq_t* rq = &g_cpu_locals[core].runqueue;
  kthread_t *current = rq->current_thread;
  if (!current) {
    sched_reschedule();
    return;
  }

  current->cpu_time_consumed++;

  if (g_policy == SCHED_POLICY_CLOUD_FAIR && current != rq->idle_thread) {
    sched_cfs_update_vruntime(rq, current, 1);
  }

  sched_update_telemetry(current);

  if (current->cpu_time_consumed >= current->time_slice_ms) {
    current->cpu_time_consumed = 0U;
    sched_reschedule();
    return;
  }

  if (g_policy != SCHED_POLICY_CLOUD_FAIR) {
      uint32_t higher_mask = (current->priority >= SCHED_MAX_PRIORITY)
                                 ? 0U
                                 : ((~0U) << (current->priority + 1U));
      if ((rq->ready_bitmap & higher_mask) != 0U) {
        sched_reschedule();
        return;
      }
  } else {
      kthread_t *next = sched_cfs_pick_next(rq);
      if (next && next->vruntime < current->vruntime) {
          sched_reschedule();
          return;
      }
  }
}

kthread_t *sched_current_thread(void) {
  return g_cpu_locals[sched_clamp_core(hal_cpu_get_id())].runqueue.current_thread;
}

kthread_t *sched_current(void) { return sched_current_thread(); }

kprocess_t *sched_current_process(void) {
  kthread_t *t = sched_current_thread();
  return t ? t->process : NULL;
}

address_space_t *sched_current_aspace(void) {
  kprocess_t *p = sched_current_process();
  return p ? p->addr_space : NULL;
}

struct capability_table *sched_current_cap_table(void) {
  kprocess_t *p = sched_current_process();
  return p ? (struct capability_table *)p->security_sandbox_ctx : NULL;
}

uint64_t sched_get_ticks(void) { return g_sched_ticks; }
void sched_set_policy(sched_policy_t policy) {
  if (policy <= SCHED_POLICY_RMS) {
    g_policy = policy;
  }
}

int sched_sys_thread_create(kprocess_t *parent, void (*entry_point)(void), uint64_t *out_tid) {
  kthread_t *t = thread_create(parent, entry_point);
  if (!t) {
    return -1;
  }
  if (out_tid) {
    *out_tid = t->thread_id;
  }
  return 0;
}

int sched_sys_thread_destroy(uint64_t tid) {
  thread_slot_t *slot = sched_find_thread_slot_by_tid(tid);
  if (!slot) {
    return -1;
  }
  return sched_mark_thread_terminated(&slot->thread);
}

int sched_sys_sleep(uint64_t millis) {
  sched_sleep(millis);
  return 0;
}

void sched_inherit_priority(kthread_t *thread, uint32_t new_priority) {
  if (!thread) {
    return;
  }
  if (new_priority > thread->priority) {
    thread->priority = new_priority;
  }
}

void sched_restore_priority(kthread_t *thread) {
  if (!thread) {
    return;
  }
  thread->priority = thread->base_priority;
}

void sched_on_mutex_wait(kthread_t *waiter, void *mutex) {
  if (!waiter || !mutex) {
    return;
  }

  waiter->waiting_on_lock = mutex;

  kthread_t *owner = sched_find_mutex_owner(mutex);
  while (owner && owner != waiter && waiter->priority > owner->priority) {
    sched_inherit_priority(owner, waiter->priority);
    if (!owner->waiting_on_lock) {
      break;
    }
    owner = sched_find_mutex_owner(owner->waiting_on_lock);
  }
}

void sched_on_mutex_acquire(kthread_t *owner, void *mutex) {
  if (!owner || !mutex) {
    return;
  }

  owner->waiting_on_lock = NULL;
  sched_register_mutex_owner(mutex, owner);
}

void sched_on_mutex_release(kthread_t *owner, void *mutex) {
  if (!owner || !mutex) {
    return;
  }

  sched_unregister_mutex_owner(mutex, owner);
  sched_restore_priority(owner);
}

kthread_t *sched_find_thread_by_id(uint64_t tid) {
  thread_slot_t *slot = sched_find_thread_slot_by_tid(tid);
  return slot ? &slot->thread : NULL;
}

int sched_adjust_priority(kthread_t *thread, uint32_t new_priority) {
  if (!thread) {
    return -1;
  }
  if (new_priority > SCHED_MAX_PRIORITY) {
    new_priority = SCHED_MAX_PRIORITY;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);

  if (slot && slot->is_on_runqueue != 0U) {
    uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
    bool is_local = (thread->bound_core_id == current_core);

    if (is_local) {
        hal_cpu_disable_interrupts();
    } else {
        // Fallback for safety. In strict multikernel, we should send an IPI to mutate.
        spin_lock(&g_cpu_locals[thread->bound_core_id].runqueue.lock);
    }

    sched_rq_t *rq = &g_cpu_locals[thread->bound_core_id].runqueue;

    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
      sched_cfs_dequeue(rq, thread);
    } else {
      list_del(&slot->run_node);
      list_init(&slot->run_node);
      sched_ready_bitmap_clear_if_empty(rq, thread->priority);
    }

    slot->is_on_runqueue = 0U;
    if (rq->runnable_count > 0U) {
      rq->runnable_count--;
    }

    if (is_local) {
        hal_cpu_enable_interrupts();
    } else {
        spin_unlock(&g_cpu_locals[thread->bound_core_id].runqueue.lock);
    }
  }

  thread->priority = new_priority;
  if (thread->state == THREAD_STATE_READY) {
    (void)sched_enqueue(thread, thread->bound_core_id); // Defer via enqueue inbox
  }
  return 0;
}

int sched_set_thread_priority(uint64_t tid, uint32_t new_priority) {
  return sched_adjust_priority(sched_find_thread_by_id(tid), new_priority);
}

int sched_sys_set_priority(uint64_t tid, uint32_t new_priority) {
  return sched_set_thread_priority(tid, new_priority);
}

int sched_migrate_task(kthread_t *thread, uint32_t new_node) {
  if (!thread || new_node >= g_active_core_count) {
    return -1;
  }
  if ((thread->affinity_mask & (1U << new_node)) == 0U) {
    return -2;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
  if (!slot) {
    return -1;
  }

  if (slot->is_on_runqueue != 0U) {
    uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
    bool is_local = (thread->bound_core_id == current_core);

    if (is_local) {
        hal_cpu_disable_interrupts();
    } else {
        spin_lock(&g_cpu_locals[thread->bound_core_id].runqueue.lock);
    }

    sched_rq_t *rq = &g_cpu_locals[thread->bound_core_id].runqueue;

    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
      sched_cfs_dequeue(rq, thread);
    } else {
      list_del(&slot->run_node);
      list_init(&slot->run_node);
      sched_ready_bitmap_clear_if_empty(rq, thread->priority);
    }

    slot->is_on_runqueue = 0U;
    if (rq->runnable_count > 0U) {
      rq->runnable_count--;
    }

    if (is_local) {
        hal_cpu_enable_interrupts();
    } else {
        spin_unlock(&g_cpu_locals[thread->bound_core_id].runqueue.lock);
    }
  }

  thread->bound_core_id = new_node;
  thread->preferred_numa_node = (uint8_t)new_node;
  if (thread->state == THREAD_STATE_READY) {
    return sched_enqueue(thread, new_node); // Enqueues into target core's inbox if remote
  }
  return 0;
}

int sched_set_thread_preferred_node(uint64_t tid, uint8_t node_id) {
  return sched_migrate_task(sched_find_thread_by_id(tid), node_id);
}

int sched_sys_set_affinity(uint64_t tid, uint32_t affinity_mask) {
  kthread_t *thread = sched_find_thread_by_id(tid);
  if (!thread || affinity_mask == 0U) {
    return -1;
  }
  thread->affinity_mask = affinity_mask;

  uint32_t current_core = thread->bound_core_id;
  if ((affinity_mask & (1U << current_core)) == 0U) {
    for (uint32_t core = 0; core < g_active_core_count; ++core) {
      if ((affinity_mask & (1U << core)) != 0U) {
        return sched_migrate_task(thread, core);
      }
    }
    return -1;
  }
  return 0;
}

int sched_throttle_core(uint32_t core_id) {
  if (core_id >= g_active_core_count) {
    return -1;
  }
  g_cpu_locals[core_id].runqueue.throttled = 1U;
  return 0;
}

int sched_request_remote_handoff(kthread_t *thread, uint32_t target_core, uint32_t auth_token) {
  if (!thread || target_core >= g_active_core_count) {
    return -1; // Invalid argument
  }

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  if (target_core == current_core) {
    return -1; // Cannot handoff to self
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
  if (!slot) {
    return -1;
  }

  hal_cpu_disable_interrupts();

  // Validate state - only READY threads can be handed off
  if (thread->state != THREAD_STATE_READY) {
    hal_cpu_enable_interrupts();
    return -2; // Invalid state
  }

  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;

  // Dequeue from local runqueue
  if (slot->is_on_runqueue != 0U) {
    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
      sched_cfs_dequeue(rq, thread);
    } else {
      list_del(&slot->run_node);
      list_init(&slot->run_node);
      sched_ready_bitmap_clear_if_empty(rq, thread->priority);
    }
    slot->is_on_runqueue = 0U;
    if (rq->runnable_count > 0U) {
      rq->runnable_count--;
    }
  }

  // Transition to pending handoff state
  thread->state = THREAD_STATE_REMOTE_HANDOFF_PENDING;

  hal_cpu_enable_interrupts();

  // Prepare and send uRPC message
  mk_channel_t channel;
  if (mk_get_channel(current_core, target_core, &channel) != 0) {
    // If channel fails, revert state and re-enqueue locally
    hal_cpu_disable_interrupts();
    thread->state = THREAD_STATE_READY;
    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
      sched_cfs_enqueue(rq, thread);
    } else {
      list_add(&slot->run_node, &rq->ready_queue[thread->priority]);
      sched_ready_bitmap_set(rq, thread->priority);
    }
    slot->is_on_runqueue = 1U;
    rq->runnable_count++;
    hal_cpu_enable_interrupts();
    return -3; // Channel error
  }

  mk_msg_thread_handoff_t payload = {
    .thread_id = thread->thread_id,
    .source_core = current_core,
    .target_core = target_core,
    .priority = thread->priority,
    .flags = 0
  };

  urpc_msg_t msg = {
    .msg_type = MK_MSG_THREAD_HANDOFF_REQ,
    .payload_size = sizeof(payload),
    .sender_core_id = current_core,
    .receiver_core_id = target_core,
    .auth_token = auth_token
  };
  __builtin_memcpy(msg.payload_data, &payload, sizeof(payload));

  int ret = mk_send_message(&channel, msg.msg_type, msg.payload_data, msg.payload_size);
  if (ret != 0) {
      // If send fails, revert state and re-enqueue locally
      hal_cpu_disable_interrupts();
      thread->state = THREAD_STATE_READY;
      if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
        sched_cfs_enqueue(rq, thread);
      } else {
        list_add(&slot->run_node, &rq->ready_queue[thread->priority]);
        sched_ready_bitmap_set(rq, thread->priority);
      }
      slot->is_on_runqueue = 1U;
      rq->runnable_count++;
      hal_cpu_enable_interrupts();
      return -4; // Send error
  }

  return 0;
}

int sched_ai_apply_suggestion(const ai_suggestion_t *suggestion) {
  if (!suggestion) {
    return -1;
  }
  kthread_t *thread = sched_find_thread_by_id((uint64_t)suggestion->target_id);
  if (!thread) {
      return -1;
  }
  switch (suggestion->action) {
  case AI_ACTION_ADJUST_PRIORITY:
    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
        if (suggestion->value == 0) return -1;
        thread->weight = suggestion->value;
        return 0;
    }
    return sched_adjust_priority(thread, suggestion->value);
  case AI_ACTION_MIGRATE_TASK:
    return sched_migrate_task(thread, suggestion->value);
  case AI_ACTION_THROTTLE_CORE:
    return sched_throttle_core(suggestion->value);
  case AI_ACTION_KILL_TASK:
    if (sched_mark_thread_terminated(thread) != 0) {
      return -1;
    }
    if (thread == sched_current_thread()) {
      uint32_t core = sched_clamp_core(hal_cpu_get_id());
      g_cpu_locals[core].runqueue.current_thread = NULL;
      sched_reschedule();
    }
    return 0;
  case AI_ACTION_NONE:
  default:
    return -1;
  }
}

int sched_enqueue_ai_suggestion(const ai_suggestion_t *suggestion) {
  if (!suggestion) {
    return -1;
  }
  uint32_t next = (g_pending_suggestions.head + 1U) % SCHED_MAX_PENDING_SUGGESTIONS;
  if (next == g_pending_suggestions.tail) {
    return -2;
  }
  g_pending_suggestions.queue[g_pending_suggestions.head] = *suggestion;
  g_pending_suggestions.head = next;
  return 0;
}

#ifdef Profile_RTOS
void sched_disable_tick_for_core(uint32_t core_id) { (void)core_id; }
#endif

void sched_notify_ipc_ready(uint32_t core_id, uint32_t msg_type) {
  (void)core_id;
  (void)msg_type;
}

int thread_raise_fault(kthread_t *thread, thread_fault_t fault) {
    if (!thread) return -1; // -EINVAL mapped

    thread->pending_fault = fault;
    thread->fault_pending = true;
    thread->state = THREAD_STATE_TERMINATED; // Mark as doomed

    /* TODO(personality/linux): translate THREAD_FAULT_SEGV / STACK_OVERFLOW to SIGSEGV */

    sched_reschedule();
    return 0;
}

static void sched_cfs_enqueue(sched_rq_t *rq, kthread_t *thread) {
    struct rb_node **link = &rq->cfs_runqueue.rb_node;
    struct rb_node *parent = NULL;
    int64_t vruntime = thread->vruntime;
    int leftmost = 1;

    // Wakeup preemption edge case: bound the negative drift
    if (vruntime < rq->min_vruntime) {
        vruntime = rq->min_vruntime;
        thread->vruntime = vruntime;
    }

    while (*link) {
        parent = *link;
        kthread_t *entry = (kthread_t *)(void *)((char *)parent - offsetof(kthread_t, cfs_node));

        if (vruntime < entry->vruntime) {
            link = &parent->rb_left;
        } else {
            link = &parent->rb_right;
            leftmost = 0;
        }
    }

    rb_link_node(&thread->cfs_node, parent, link);
    rb_insert_color(&thread->cfs_node, &rq->cfs_runqueue);

    // Update min_vruntime if this is the new leftmost node
    if (leftmost) {
        rq->min_vruntime = vruntime;
    }
}

static void sched_cfs_dequeue(sched_rq_t *rq, kthread_t *thread) {
    if (rq->cfs_runqueue.rb_node == NULL) {
        return;
    }

    // Is it the leftmost?
    int leftmost = (rb_first(&rq->cfs_runqueue) == &thread->cfs_node);

    rb_erase(&thread->cfs_node, &rq->cfs_runqueue);

    if (leftmost) {
        struct rb_node *first = rb_first(&rq->cfs_runqueue);
        if (first) {
            kthread_t *next = (kthread_t *)(void *)((char *)first - offsetof(kthread_t, cfs_node));
            rq->min_vruntime = next->vruntime;
        }
    }
}

static kthread_t *sched_cfs_pick_next(sched_rq_t *rq) {
    struct rb_node *left = rb_first(&rq->cfs_runqueue);
    if (!left) {
        return NULL;
    }
    return (kthread_t *)(void *)((char *)left - offsetof(kthread_t, cfs_node));
}

static void sched_cfs_update_vruntime(sched_rq_t *rq, kthread_t *thread, uint64_t delta_exec) {
    if (delta_exec == 0) return;

    // Validate monotonic growth
    int64_t prev_vruntime = thread->vruntime;

    // vruntime += (delta_exec * NICE_0_WEIGHT) / weight
    uint32_t weight = thread->weight > 0 ? thread->weight : 1;
    uint64_t delta_vruntime = (delta_exec * CFS_NICE_0_WEIGHT) / weight;

    thread->vruntime += delta_vruntime;

    // Ensure monotonic
    if (thread->vruntime < prev_vruntime) {
        thread->vruntime = prev_vruntime; // Handle theoretical wrap around
    }

    // Track min_vruntime to prevent unbounded lag
    if (thread->vruntime < rq->min_vruntime) {
         // Should not happen, but invariant safety
         rq->min_vruntime = thread->vruntime;
    } else if (thread->vruntime > rq->min_vruntime && rq->runnable_count == 1) {
         // Single active task drags the baseline forward
         rq->min_vruntime = thread->vruntime;
    }
}

static void sched_validate_rq(sched_rq_t *rq) {
    // Debug only
#ifndef NDEBUG
    if (!rq) return;

    // Valid count
    if (rq->runnable_count > SCHED_MAX_THREADS) {
        kernel_panic("Runqueue count invalid/underflow");
    }

    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
        // Validate min_vruntime is sensible
        struct rb_node *first = rb_first(&rq->cfs_runqueue);
        if (first) {
            kthread_t *next = (kthread_t *)(void *)((char *)first - offsetof(kthread_t, cfs_node));
            if (next->vruntime < rq->min_vruntime && rq->min_vruntime - next->vruntime > 1000) {
                // Minor drift is okay due to rounding, but major divergence is a bug
                kernel_panic("Runqueue min_vruntime divergence");
            }
        }
    } else {
        // Validate priority bitmaps vs lists
        for (uint32_t i = 0; i < MAX_PRIORITY_LEVELS; i++) {
            int is_empty = list_empty(&rq->ready_queue[i]);
            int bit_set = (rq->ready_bitmap & (1U << i)) != 0;
            if (is_empty && bit_set) {
                 kernel_panic("Runqueue bitmap indicates ready task but list is empty");
            } else if (!is_empty && !bit_set) {
                 kernel_panic("Runqueue list has tasks but bitmap bit is cleared");
            }
        }
    }
#else
    (void)rq;
#endif
}
