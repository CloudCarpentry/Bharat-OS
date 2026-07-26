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
#include "lib/base/string.h"

#include "sched_internal.h"

#define SCHED_DEFAULT_SLICE_MS 10U

// Removed core_runqueue_t definition from here as it's now in sched.h as sched_rq_t
// Removed static core_runqueue_t g_runqueues

sched_policy_t g_policy = SCHED_POLICY_PRIORITY;
static volatile uint64_t g_next_thread_id = 1U;
static volatile uint64_t g_next_process_id = 1U;




uint8_t g_sched_initialized = 0U;
uint32_t g_active_core_count = 1U;

#if defined(BHARAT_ENABLE_KERNEL_SELFTESTS)
uint32_t g_sched_test_core_count = 1U;
#endif

enum {
  SCHED_BOOTSTRAP_IDLE = 0,
  SCHED_BOOTSTRAP_MONITOR = 1,
  SCHED_BOOTSTRAP_THREAD_TYPES = 2
};

void fv_secure_context_switch(void *next_thread_frame) __attribute__((weak));
void sched_ready_bitmap_set(sched_rq_t *rq, uint32_t prio);
void sched_ready_bitmap_clear_if_empty(sched_rq_t *rq, uint32_t prio);
int sched_pick_priority_from_bitmap(const sched_rq_t *rq, int highest);

// CFS Functions
void sched_cfs_enqueue(sched_rq_t *rq, bh_thread_t *thread);
void sched_cfs_dequeue(sched_rq_t *rq, bh_thread_t *thread);
bh_thread_t *sched_cfs_pick_next(sched_rq_t *rq);
void sched_cfs_update_vruntime(sched_rq_t *rq, bh_thread_t *thread, uint64_t delta_exec);

// EDF Functions
void sched_edf_enqueue(sched_rq_t *rq, bh_thread_t *thread);
void sched_edf_dequeue(sched_rq_t *rq, bh_thread_t *thread);
bh_thread_t *sched_edf_pick_next(sched_rq_t *rq);

void sched_validate_rq(sched_rq_t *rq);

uint32_t sched_clamp_core(uint32_t core_id) {
  if (core_id >= g_active_core_count) {
    return 0U;
  }
  return core_id;
}

#include "hal/hal_discovery.h"

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
  system_discovery_t* discovery = hal_get_system_discovery();
  if (discovery && discovery->topology.cpu_count > 0) {
    uint32_t count = discovery->topology.cpu_count;
    if (count > MAX_SUPPORTED_CORES) {
        count = MAX_SUPPORTED_CORES;
    }
    return count;
  }
  return 1U;
#endif
}



thread_slot_t *sched_find_thread_slot_by_tid_local(sched_rq_t *rq, uint64_t tid) {
  uint16_t home_core = bh_tid_home_core(tid);
  uint16_t slot_idx = bh_tid_slot(tid);
  if (home_core >= g_active_core_count || !rq || rq != &g_cpu_locals[home_core].runqueue) {
    return NULL;
  }
  if (slot_idx < SCHED_MAX_THREADS) {
    thread_slot_t *slots = (thread_slot_t *)rq->threads;
    if (slots && slots[slot_idx].in_use && slots[slot_idx].thread.thread_id == tid) {
      return &slots[slot_idx];
    }
  } else if (slot_idx >= SCHED_MAX_THREADS && slot_idx < SCHED_MAX_THREADS + SCHED_BOOTSTRAP_THREAD_TYPES) {
    thread_slot_t *slots = (thread_slot_t *)rq->bootstrap_threads;
    uint32_t b_idx = slot_idx - SCHED_MAX_THREADS;
    if (slots && slots[b_idx].in_use && slots[b_idx].thread.thread_id == tid) {
      return &slots[b_idx];
    }
  }
  return NULL;
}

thread_slot_t *sched_find_thread_slot_by_tid(uint64_t tid) {
  uint16_t home_core = bh_tid_home_core(tid);
  if (home_core >= g_active_core_count) {
    return NULL;
  }
  sched_rq_t *rq = &g_cpu_locals[home_core].runqueue;
  return sched_find_thread_slot_by_tid_local(rq, tid);
}

static thread_slot_t *sched_find_free_thread_slot(void) {
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;

  if (rq->free_thread_head == UINT32_MAX) {
    return NULL;
  }
  uint32_t idx = rq->free_thread_head;
  thread_slot_t *slots = (thread_slot_t *)rq->threads;
  rq->free_thread_head = slots[idx].next_free;
  return &slots[idx];
}

static process_slot_t *sched_find_free_process_slot(void) {
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;

  if (rq->free_process_head == UINT32_MAX) {
    return NULL;
  }
  uint32_t idx = rq->free_process_head;
  process_slot_t *slots = (process_slot_t *)rq->processes;
  rq->free_process_head = slots[idx].next_free;
  return &slots[idx];
}

void sched_sleep_enqueue(thread_slot_t *slot, uint32_t core_id) {
  if (!slot || slot->is_sleeping != 0U || slot->is_blocked != 0U) {
    return;
  }
  list_add(&slot->wait_node, &g_cpu_locals[core_id].runqueue.sleeping_list);
  slot->is_sleeping = 1U;
}

void sched_sleep_dequeue(thread_slot_t *slot) {
  if (!slot || slot->is_sleeping == 0U) {
    return;
  }
  list_del(&slot->wait_node);
  list_init(&slot->wait_node);
  slot->is_sleeping = 0U;
}

void sched_block_enqueue(thread_slot_t *slot, uint32_t core_id) {
  if (!slot || slot->is_sleeping != 0U || slot->is_blocked != 0U) {
    return;
  }
  list_add(&slot->wait_node, &g_cpu_locals[core_id].runqueue.blocked_list);
  slot->is_blocked = 1U;
}

void sched_block_dequeue(thread_slot_t *slot) {
  if (!slot || slot->is_blocked == 0U) {
    return;
  }
  list_del(&slot->wait_node);
  list_init(&slot->wait_node);
  slot->is_blocked = 0U;
}

void sched_detach_thread_from_queues(thread_slot_t *slot) {
  if (!slot) {
    return;
  }
  bh_thread_t *thread = &slot->thread;
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());

  // Assert local ownership and local runqueue
  if (thread->owner_cpu != current_core &&
      thread->owner_state != THREAD_OWNER_REMOTE_PENDING) {
      kernel_panic("sched_detach_thread_from_queues failed: not local owner");
  }

  sched_rq_t *rq = sched_local_rq();
  sched_assert_local_rq(rq);

  hal_cpu_disable_interrupts();

  if (slot->is_on_runqueue != 0U) {
    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
      sched_cfs_dequeue(rq, thread);
    } else if (g_policy == SCHED_POLICY_EDF) {
      sched_edf_dequeue(rq, thread);
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

  hal_cpu_enable_interrupts();
}

int sched_enqueue_reap(thread_slot_t *slot) {
  if (!slot || slot->is_bootstrap != 0U) {
    return -1;
  }

  uint32_t core_id = sched_clamp_core(slot->creation_core_id);
  sched_rq_t *rq = &g_cpu_locals[core_id].runqueue;

  spin_lock(&rq->lock);
  if (slot->reap_pending != 0U) {
    spin_unlock(&rq->lock);
    return 0;
  }

  uint32_t idx = (uint32_t)(slot - (thread_slot_t*)rq->threads);
  slot->reap_pending = 1U;
  slot->reap_next = UINT32_MAX;
  if (rq->reap_tail == UINT32_MAX) {
    rq->reap_head = idx;
    rq->reap_tail = idx;
  } else {
    ((thread_slot_t*)rq->threads)[rq->reap_tail].reap_next = idx;
    rq->reap_tail = idx;
  }
  spin_unlock(&rq->lock);
  return 0;
}

int sched_mark_thread_terminated(bh_thread_t *thread) {
  if (!thread) {
    return -1;
  }
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  if (__atomic_load_n(&thread->owner_cpu, __ATOMIC_ACQUIRE) != current_core) {
      kernel_panic("sched_mark_thread_terminated: executing on non-owner CPU");
  }
  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
  if (!slot) {
    return -1;
  }
  if (thread->state == THREAD_STATE_TERMINATED) {
    if (thread->home_core_id == current_core) {
      return sched_enqueue_reap(slot);
    }
    return 0;
  }

  thread->state = THREAD_STATE_TERMINATED;
  __atomic_store_n(&thread->owner_state, THREAD_OWNER_NONE, __ATOMIC_RELEASE);

  if (thread != sched_current_thread()) {
    sched_detach_thread_from_queues(slot);
  }

  if (thread->home_core_id == current_core) {
    return sched_enqueue_reap(slot);
  } else {
    sched_remote_cmd_t *cmd = sched_allocate_outbound_cmd();
    if (cmd) {
      cmd->type = SCHED_REMOTE_REAP;
      cmd->source_cpu = current_core;
      cmd->target_cpu = thread->home_core_id;
      cmd->thread_id = thread->thread_id;
      cmd->expected_thread_generation = thread->sched_generation;
      cmd->state = SCHED_REMOTE_CMD_PENDING;
      kstatus_t status = sched_remote_submit(thread->home_core_id, cmd);
      if (status != K_OK) {
        sched_remote_cmd_release(cmd);
        return status;
      }
    }
  }
  return 0;
}

void sched_reap_terminated_threads(void) {
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;

  while (1) {
    thread_slot_t *slot = NULL;

    spin_lock(&rq->lock);
    if (rq->reap_head != UINT32_MAX) {
      uint32_t idx = rq->reap_head;
      slot = &((thread_slot_t*)rq->threads)[idx];
      rq->reap_head = slot->reap_next;
      if (rq->reap_head == UINT32_MAX) {
        rq->reap_tail = UINT32_MAX;
      }
      slot->reap_next = UINT32_MAX;
      slot->reap_pending = 0U;
    }
    spin_unlock(&rq->lock);

    if (!slot) {
      break;
    }
    (void)thread_destroy(&slot->thread);
  }
}

extern bool sched_is_core_admissible(bh_thread_t *t, int cpu_id);



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
    bh_thread_yield();
  }
}

void sched_thread_exit_trampoline(void) {
  bh_thread_t *current = sched_current_thread();
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

void sched_reset_core_runqueues(void) {
  for (uint32_t core = 0; core < g_active_core_count; ++core) {
    sched_rq_t *rq = &g_cpu_locals[core].runqueue;
    rq->current_thread = NULL;
    rq->idle_thread = NULL;
    g_cpu_locals[core].runqueue.total_ticks = 0U;
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

    sched_cmd_ring_init(&rq->remote.cmd_ring, rq->remote.cmd_slots, SCHED_REMOTE_CMD_CAPACITY);
    sched_completion_ring_init(&rq->remote.completion_ring, rq->remote.completion_slots, SCHED_REMOTE_CMD_CAPACITY);
    rq->remote.resched_pending = 0U;
    rq->remote.submitted = 0U;
    rq->remote.consumed = 0U;
    rq->remote.full = 0U;
    rq->remote.ipi_sent = 0U;
    rq->remote.ipi_coalesced = 0U;

    for (uint32_t w = 0; w < SCHED_CMD_BITMAP_WORDS; ++w) {
        rq->outbound_alloc_bitmap[w] = 0U;
    }

    for (uint32_t i = 0; i < SCHED_REMOTE_CMD_CAPACITY; ++i) {
        rq->outbound_cmds[i].handle.slot = i;
        rq->outbound_cmds[i].handle.origin_cpu = core;
        rq->outbound_cmds[i].handle.generation = 1;
        rq->outbound_cmds[i].state = SCHED_REMOTE_CMD_EMPTY;
    }

    rq->load_snapshot.runnable_count = 0;
    rq->load_snapshot.load_seq = 0;
    for (uint32_t p = 0; p < MAX_PRIORITY_LEVELS; ++p) {
      list_init(&rq->ready_queue[p]);
    }
    rq->ready_bitmap = 0U;
    rq->edf_runqueue.rb_node = NULL;
    rq->rt_budget_used = 0U;
    rq->rt_budget_total = 0U;

    rq->reap_head = UINT32_MAX;
    rq->reap_tail = UINT32_MAX;

    if (!rq->threads) rq->threads = (struct thread_slot*)kmalloc(sizeof(thread_slot_t) * SCHED_MAX_THREADS);
    if (!rq->processes) rq->processes = (struct process_slot*)kmalloc(sizeof(process_slot_t) * SCHED_MAX_PROCESSES);
    if (!rq->bootstrap_threads) rq->bootstrap_threads = (struct thread_slot*)kmalloc(sizeof(thread_slot_t) * SCHED_BOOTSTRAP_THREAD_TYPES);
    if (!rq->bootstrap_stacks) rq->bootstrap_stacks = (uint8_t*)kmalloc(16384U * SCHED_BOOTSTRAP_THREAD_TYPES);
    if (!rq->mutex_owners) rq->mutex_owners = kmalloc(sizeof(mutex_owner_entry_t) * SCHED_MAX_THREADS);
    if (!rq->pending_suggestions) rq->pending_suggestions = kmalloc(sizeof(suggestion_queue_t));

    if (!rq->threads || !rq->processes || !rq->bootstrap_threads || !rq->bootstrap_stacks || !rq->mutex_owners || !rq->pending_suggestions) {
        kernel_panic("sched_reset_core_runqueues kmalloc failed");
    }

    rq->free_thread_head = 0U;
    for (size_t i = 0; i < SCHED_MAX_THREADS; ++i) {
        ((thread_slot_t*)rq->threads)[i].in_use = 0U;
        ((thread_slot_t*)rq->threads)[i].is_bootstrap = 0U;
        ((thread_slot_t*)rq->threads)[i].generation = 0U;
        ((thread_slot_t*)rq->threads)[i].next_free = (i + 1U < SCHED_MAX_THREADS) ? (uint32_t)(i + 1U) : UINT32_MAX;
        ((thread_slot_t*)rq->threads)[i].reap_next = UINT32_MAX;
        ((thread_slot_t*)rq->threads)[i].reap_pending = 0U;
    }

    rq->free_process_head = 0U;
    for (size_t i = 0; i < SCHED_MAX_PROCESSES; ++i) {
        ((process_slot_t*)rq->processes)[i].in_use = 0U;
        ((process_slot_t*)rq->processes)[i].next_free = (i + 1U < SCHED_MAX_PROCESSES) ? (uint32_t)(i + 1U) : UINT32_MAX;
    }

    memset(rq->bootstrap_threads, 0, sizeof(thread_slot_t) * SCHED_BOOTSTRAP_THREAD_TYPES);
    memset(rq->mutex_owners, 0, sizeof(mutex_owner_entry_t) * SCHED_MAX_THREADS);
    memset(rq->pending_suggestions, 0, sizeof(suggestion_queue_t));
  }
}

static bh_thread_t *sched_create_bootstrap_thread(bh_process_t *parent,
                                                uint32_t core,
                                                uint32_t kind,
                                                void (*entry_point)(void),
                                                uint32_t priority,
                                                uint8_t enqueue) {
  sched_rq_t *rq = &g_cpu_locals[core].runqueue;
  thread_slot_t *slot = &((thread_slot_t*)rq->bootstrap_threads)[kind];
  memset(slot, 0, sizeof(*slot));
  slot->in_use = 1U;
  slot->is_bootstrap = 1U;
  slot->generation = 1U;

  uint32_t slot_idx = SCHED_MAX_THREADS + kind;
  slot->thread.thread_id =
      ((uint64_t)slot->generation << 32)
    | ((uint64_t)core << 16)
    | slot_idx;

  slot->thread.process_id = parent ? parent->process_id : 0U;
  slot->thread.process = parent;
  slot->thread.constraints.cpu_mask = 0xFFFFFFFF; // Admissible everywhere by default
  slot->thread.home_core_id = core;
  slot->thread.generation = 1U;
  slot->thread.sched_generation = 1U;
  slot->thread.personality = BH_PERSONALITY_NATIVE;
  slot->thread.state = THREAD_STATE_READY;
  slot->thread.priority = priority;
  slot->thread.base_priority = priority;
  slot->thread.time_slice_ms = SCHED_DEFAULT_SLICE_MS;
  slot->thread.bound_core_id = core;
  slot->thread.affinity_mask = (1U << core);
  slot->thread.cpu_context = &slot->context;

  uint8_t *stacks = (uint8_t*)rq->bootstrap_stacks;
  slot->thread.kernel_stack = (virt_addr_t)(uintptr_t)&stacks[kind * 16384U];

  arch_prepare_initial_context(
      &slot->context, entry_point,
      (uint64_t)(uintptr_t)&stacks[kind * 16384U] + 16384U);

  ai_sched_init_context(&slot->ai_ctx);
  slot->ai_ctx.thread_id = (uint32_t)slot->thread.thread_id;
  slot->thread.ai_sched_ctx = &slot->ai_ctx;
  list_init(&slot->run_node);
  list_init(&slot->wait_node);

  if (kind == SCHED_BOOTSTRAP_IDLE) {
    slot->thread.flags |= BH_THREAD_FLAG_IDLE;
  }

  if (enqueue != 0U) {
    (void)sched_enqueue(&slot->thread, core);
  }
  return &slot->thread;
}

void sched_init(void) {
  if (g_sched_initialized != 0U) {
#if defined(BHARAT_ENABLE_KERNEL_SELFTESTS)
    extern void sched_test_reset(void);
    sched_test_reset();
#else
    return;
#endif
  }

  g_active_core_count = sched_configured_core_count();

  g_next_thread_id = 1U;
  g_next_process_id = 1U;





  sched_reset_core_runqueues();

  bh_process_t *idle_process = process_create("idle_process");
  for (uint32_t core = 0; core < g_active_core_count; ++core) {
    bh_thread_t *idle = sched_create_bootstrap_thread(
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

bh_process_t *process_create(const char *name) {
  (void)name;
  process_slot_t *slot = sched_find_free_process_slot();
  if (!slot) {
    return NULL;
  }

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;

  slot->in_use = 1U;
  slot->process.process_id = atomic64_fetch_and_add_ptr(&g_next_process_id, 1);
  slot->process.addr_space = mm_create_address_space();
  slot->process.main_thread = NULL;
  slot->process.security_sandbox_ctx = NULL;

  // Explicit multikernel ownership metadata
  slot->process.owner_core_id = hal_cpu_get_id();
  slot->process.object_id = slot->process.process_id;

  if (!slot->process.addr_space || cap_table_init_for_process(&slot->process) != 0) {
    slot->in_use = 0U;
    uint32_t idx = (uint32_t)(slot - (process_slot_t*)rq->processes);
    slot->next_free = rq->free_process_head;
    rq->free_process_head = idx;
    return NULL;
  }

  return &slot->process;
}



bh_thread_t *thread_create_detached(bh_process_t *parent, void (*entry_point)(void)) {
  thread_slot_t *slot = sched_find_free_thread_slot();
  if (!slot) {
    return NULL;
  }

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;

  uint32_t slot_idx = (uint32_t)(slot - (thread_slot_t*)rq->threads);
  uint32_t prev_gen = slot->generation;
  memset(slot, 0, sizeof(*slot));
  slot->generation = prev_gen + 1U;
  if (slot->generation == 0U) {
    slot->generation++;
  }
  slot->in_use = 1U;

  slot->thread.thread_id =
      ((uint64_t)slot->generation << 32)
    | ((uint64_t)current_core << 16)
    | slot_idx;

  slot->thread.process_id = parent ? parent->process_id : 0U;
  slot->thread.process = parent;
  slot->thread.home_core_id = current_core;
  slot->thread.generation = slot->generation;
  slot->thread.sched_generation = slot->generation;
  slot->thread.personality = BH_PERSONALITY_NATIVE;
  slot->thread.state = THREAD_STATE_READY;
  slot->thread.priority = 1U;
  slot->thread.base_priority = 1U;
  slot->thread.cpu_time_consumed = 0U;
  slot->thread.time_slice_ms = SCHED_DEFAULT_SLICE_MS;
  slot->thread.preferred_numa_node = 0U;
  slot->thread.bound_core_id = sched_clamp_core(hal_cpu_get_id());
  slot->thread.affinity_mask = SCHED_AFFINITY_ANY;

  // Initialize constraints with sane defaults
  slot->thread.constraints.cpu_mask = SCHED_AFFINITY_ANY;
  slot->thread.constraints.flags = 0;
  slot->thread.constraints.latency_class = 0;
  slot->thread.constraints.energy_class = 0;

  slot->thread.priority = 1U; // Default priority
  slot->thread.base_priority = 1U;
  slot->thread.absolute_deadline_ms = 0; // 0 indicates no absolute deadline set yet

  // RT attributes defaults
  slot->thread.rt_attr.wcet_ms = 0;
  slot->thread.rt_attr.period_ms = 0;
  slot->thread.rt_attr.deadline_ms = 0;

  slot->thread.wake_deadline_ms = 0U;
  slot->thread.context_switch_count = 0U;
  slot->creation_core_id = sched_clamp_core(hal_cpu_get_id());

  #define KERNEL_STACK_SIZE 16384U
  void *stack = kmalloc(KERNEL_STACK_SIZE);
  if (!stack) {
    slot->in_use = 0U;
    uint32_t idx = (uint32_t)(slot - (thread_slot_t*)rq->threads);
    slot->next_free = rq->free_thread_head;
    rq->free_thread_head = idx;
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
    uint32_t idx = (uint32_t)(slot - (thread_slot_t*)rq->threads);
    slot->next_free = rq->free_thread_head;
    rq->free_thread_head = idx;
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

bh_thread_t *thread_create(bh_process_t *parent, void (*entry_point)(void)) {
  bh_thread_t *thread = thread_create_detached(parent, entry_point);
  if (thread && entry_point != (void (*)(void))sched_idle_task) {
    (void)sched_enqueue(thread, thread->bound_core_id);
  }
  return thread;
}



bh_thread_t *sched_find_mutex_owner(void *mutex) {
  if (!mutex) {
    return NULL;
  }
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;
  mutex_owner_entry_t *owners = (mutex_owner_entry_t *)rq->mutex_owners;

  for (size_t i = 0; i < SCHED_MAX_THREADS; ++i) {
    if (owners[i].mutex == mutex) {
      return owners[i].owner;
    }
  }

  return NULL;
}

void sched_register_mutex_owner(void *mutex, bh_thread_t *owner) {
  if (!mutex) {
    return;
  }
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;
  mutex_owner_entry_t *owners = (mutex_owner_entry_t *)rq->mutex_owners;

  for (size_t i = 0; i < SCHED_MAX_THREADS; ++i) {
    if (owners[i].mutex == mutex || owners[i].mutex == NULL) {
      owners[i].mutex = mutex;
      owners[i].owner = owner;
      return;
    }
  }
}

void sched_unregister_mutex_owner(void *mutex, bh_thread_t *owner) {
  if (!mutex) {
    return;
  }
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;
  mutex_owner_entry_t *owners = (mutex_owner_entry_t *)rq->mutex_owners;

  for (size_t i = 0; i < SCHED_MAX_THREADS; ++i) {
    if (owners[i].mutex == mutex &&
        (owner == NULL || owners[i].owner == owner)) {
      owners[i].mutex = NULL;
      owners[i].owner = NULL;
      return;
    }
  }
}













void sched_update_telemetry(bh_thread_t *thread) {
  if (!thread || !thread->ai_sched_ctx) {
    return;
  }
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  ai_sched_collect_sample(thread->ai_sched_ctx, thread->time_slice_ms,
                          thread->cpu_time_consumed,
                          sched_run_queue_depth(core),
                          (uint32_t)thread->context_switch_count);
}

bh_thread_t *sched_pick_next_ready(uint32_t core_id) {
  core_id = sched_clamp_core(core_id);
  sched_rq_t *rq = &g_cpu_locals[core_id].runqueue;

  bh_thread_t *next = NULL;

  if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
      next = sched_cfs_pick_next(rq);
      if (next) {
          sched_invariant_on_dequeue(next);
          sched_cfs_dequeue(rq, next);
      }
  } else if (g_policy == SCHED_POLICY_EDF) {
      next = sched_edf_pick_next(rq);
      if (next) {
          sched_invariant_on_dequeue(next);
          sched_edf_dequeue(rq, next);
      }
  } else {
      int pick_highest = (g_policy == SCHED_POLICY_ROUND_ROBIN) ? 0 : 1;
      int prio = sched_pick_priority_from_bitmap(rq, pick_highest);
      if (prio >= 0) {
          list_head_t *head = &rq->ready_queue[prio];
          list_head_t *node = head->prev;
          thread_slot_t *slot = (thread_slot_t *)(void *)((char *)node - offsetof(thread_slot_t, run_node));
          sched_invariant_on_dequeue(&slot->thread);
          list_del(node);
          list_init(node);
          sched_ready_bitmap_clear_if_empty(rq, (uint32_t)prio);
          next = &slot->thread;
      }
  }

  if (!next) {
      return rq->idle_thread;
  }

  // Fallback: If not admissible on this core (e.g. from dynamic constraint update while queued),
  // try to find a valid core, else fallback to idle.
  if (!sched_is_core_admissible(next, core_id)) {
      bool found = false;
      for (uint32_t i = 0; i < g_active_core_count; ++i) {
          if (sched_is_core_admissible(next, i)) {
              sched_enqueue(next, i);
              found = true;
              break;
          }
      }
      if (!found) {
          // If no core is valid, put it back to sleep/deferred queue (simple drop for MVP)
      }
      return rq->idle_thread;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid(next->thread_id);
  if (slot) {
      slot->is_on_runqueue = 0U;
  }
  if (rq->runnable_count > 0U) {
    rq->runnable_count--;
  }

  return next;
}





bh_thread_t *sched_pick_next_ready_l0(uint32_t core_id) {
  return sched_pick_next_ready(core_id);
}





bh_thread_t *sched_pick_next_ready_l1(uint32_t core_id) {
  return sched_pick_next_ready(core_id);
}

void sched_switch_to(bh_thread_t *next, uint32_t core_id) {
  if (!next) {
    hal_cpu_enable_interrupts();
    return;
  }

  bh_thread_t *current = g_cpu_locals[core_id].runqueue.current_thread;
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
        sched_invariant_on_enqueue(current, core_id);
        if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
            sched_cfs_enqueue(rq, current);
        } else if (g_policy == SCHED_POLICY_EDF) {
            sched_edf_enqueue(rq, current);
        } else {
            list_add(&slot->run_node, &rq->ready_queue[current->priority]);
            sched_ready_bitmap_set(rq, current->priority);
        }
        slot->is_on_runqueue = 1U;
        rq->runnable_count++;
        sched_validate_rq(rq);
    }
  }

  sched_invariant_on_switch(current, next, core_id);

  next->state = THREAD_STATE_RUNNING;
  next->context_switch_count++;
  rq->context_switches++;
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















bh_thread_t *sched_edf_pick_next(sched_rq_t *rq) {
    struct rb_node *left = rb_first(&rq->edf_runqueue);
    if (!left) {
        return NULL;
    }
    return (bh_thread_t *)(void *)((char *)left - offsetof(bh_thread_t, edf_node));
}





void bh_thread_yield(void) { sched_reschedule(); }












bh_thread_t *sched_current_thread(void) {
  return g_cpu_locals[sched_clamp_core(hal_cpu_get_id())].runqueue.current_thread;
}

bh_thread_t *sched_current(void) { return sched_current_thread(); }

bh_process_t *sched_current_process(void) {
  bh_thread_t *t = sched_current_thread();
  return t ? t->process : NULL;
}

address_space_t *sched_current_aspace(void) {
  bh_process_t *p = sched_current_process();
  return p ? p->addr_space : NULL;
}

struct capability_table *sched_current_cap_table(void) {
  bh_process_t *p = sched_current_process();
  return p ? (struct capability_table *)p->security_sandbox_ctx : NULL;
}

uint64_t sched_get_ticks(void) { return g_cpu_locals[sched_clamp_core(hal_cpu_get_id())].runqueue.total_ticks; }






int sched_sys_sleep(uint64_t millis) {
  sched_sleep(millis);
  return 0;
}











bh_thread_t *sched_find_thread_by_id(uint64_t tid) {
  thread_slot_t *slot = sched_find_thread_slot_by_tid(tid);
  return slot ? &slot->thread : NULL;
}





















#ifdef Profile_RTOS
void sched_disable_tick_for_core(uint32_t core_id) { (void)core_id; }
#endif









bh_thread_t *sched_cfs_pick_next(sched_rq_t *rq) {
    struct rb_node *left = rb_first(&rq->cfs_runqueue);
    if (!left) {
        return NULL;
    }
    return (bh_thread_t *)(void *)((char *)left - offsetof(bh_thread_t, cfs_node));
}

kstatus_t sched_get_thread_snapshot(uint64_t tid, sched_thread_snapshot_t *out) {
  if (!out) return K_ERR_INVALID_ARG;
  bh_thread_t *thread = sched_find_thread_by_id(tid);
  if (!thread) return -1;

  out->thread_id = thread->thread_id;
  out->home_core_id = thread->home_core_id;
  out->bound_core_id = thread->bound_core_id;
  out->owner_cpu = __atomic_load_n(&thread->owner_cpu, __ATOMIC_ACQUIRE);
  out->state = (uint32_t)thread->state;
  out->priority = thread->priority;
  out->affinity_mask = thread->affinity_mask;
  out->migration_state = (uint32_t)__atomic_load_n(&thread->migration_state, __ATOMIC_ACQUIRE);
  out->migration_epoch = __atomic_load_n(&thread->migration_epoch, __ATOMIC_ACQUIRE);
  return K_OK;
}

int sched_set_constraints(uint64_t tid, const bh_exec_constraints_k_t *c) {
  if (!c) return -1;
  bh_thread_t *thread = sched_find_thread_by_id(tid);
  if (!thread) return -1;

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  uint32_t owner = __atomic_load_n(&thread->owner_cpu, __ATOMIC_ACQUIRE);

  if (owner != current_core) {
      sched_remote_cmd_t *cmd = sched_allocate_outbound_cmd();
      if (!cmd) return K_ERR_NO_RESOURCES;
      cmd->type = SCHED_REMOTE_SET_CONSTRAINTS;
      cmd->source_cpu = current_core;
      cmd->target_cpu = owner;
      cmd->thread_id = thread->thread_id;
      cmd->expected_thread_generation = thread->sched_generation;
      cmd->constraints = *c;
      cmd->state = SCHED_REMOTE_CMD_PENDING;

      kstatus_t status = sched_remote_submit(owner, cmd);
      if (status != K_OK) {
          sched_remote_cmd_release(cmd);
          return status;
      }
      return 0;
  }

  thread->constraints = *c;
  return 0;
}

int sched_get_constraints(uint64_t tid, bh_exec_constraints_k_t *c) {
  if (!c) return -1;
  bh_thread_t *thread = sched_find_thread_by_id(tid);
  if (!thread) return -1;
  *c = thread->constraints;
  return 0;
}

bool sched_thread_exists(uint64_t tid) {
  return (sched_find_thread_by_id(tid) != NULL);
}


