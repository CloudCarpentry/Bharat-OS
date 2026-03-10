#include "sched.h"

#include "advanced/algo_matrix.h"
#include "advanced/formal_verif.h"
#include "capability.h"
#include "hal/hal.h"
#include "kernel_safety.h"
#include "list.h"

#include <stddef.h>
#include <stdint.h>

#define MAX_SUPPORTED_CORES 8U
#define SCHED_MAX_THREADS 128U
#define SCHED_MAX_PROCESSES 32U
#define SCHED_DEFAULT_SLICE_MS 10U
#define MAX_PRIORITY_LEVELS (SCHED_MAX_PRIORITY + 1U)
#define SCHED_MAX_PENDING_SUGGESTIONS 64U
#define SCHED_AFFINITY_ANY 0xFFFFFFFFU

typedef struct {
  uint8_t in_use;
  uint32_t next_free;
  kthread_t thread;
  cpu_context_t context;
  ai_sched_context_t ai_ctx;
  list_head_t run_node;
  list_head_t sleep_node;
  uint8_t is_on_runqueue;
  uint8_t is_sleeping;
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

typedef struct {
  kthread_t *current_thread;
  kthread_t *idle_thread;
  list_head_t ready_queue[MAX_PRIORITY_LEVELS];
  list_head_t sleeping_list;
  list_head_t blocked_list;
  uint64_t total_ticks;
  uint64_t context_switches;
  uint32_t throttled;
} core_runqueue_t;

static core_runqueue_t g_runqueues[MAX_SUPPORTED_CORES];
static thread_slot_t g_threads[SCHED_MAX_THREADS];
static process_slot_t g_processes[SCHED_MAX_PROCESSES];

static sched_policy_t g_policy = SCHED_POLICY_PRIORITY;
static uint64_t g_next_thread_id = 1U;
static uint64_t g_next_process_id = 1U;
static uint64_t g_sched_ticks = 0U;
static uint64_t g_sched_context_switches = 0U;
static suggestion_queue_t g_pending_suggestions;

static mutex_owner_entry_t g_mutex_owners[SCHED_MAX_THREADS];

static uint32_t g_free_thread_head = UINT32_MAX;
static uint32_t g_free_process_head = UINT32_MAX;

void fv_secure_context_switch(void *next_thread_frame) __attribute__((weak));

static uint32_t sched_clamp_core(uint32_t core_id) {
  if (core_id >= MAX_SUPPORTED_CORES) {
    return 0U;
  }
  return core_id;
}

static thread_slot_t *sched_find_thread_slot_by_tid(uint64_t tid) {
  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
    if (g_threads[i].in_use != 0U && g_threads[i].thread.thread_id == tid) {
      return &g_threads[i];
    }
  }
  return NULL;
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
  if (!slot || slot->is_sleeping != 0U) {
    return;
  }
  list_add(&slot->sleep_node, &g_runqueues[core_id].sleeping_list);
  slot->is_sleeping = 1U;
}

static void sched_sleep_dequeue(thread_slot_t *slot) {
  if (!slot || slot->is_sleeping == 0U) {
    return;
  }
  list_del(&slot->sleep_node);
  list_init(&slot->sleep_node);
  slot->is_sleeping = 0U;
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

  if (slot->is_on_runqueue != 0U) {
    list_del(&slot->run_node);
    list_init(&slot->run_node);
    slot->is_on_runqueue = 0U;
  }

  thread->bound_core_id = core_id;
  thread->state = THREAD_STATE_READY;
  list_add(&slot->run_node, &g_runqueues[core_id].ready_queue[thread->priority]);
  slot->is_on_runqueue = 1U;
  return 0;
}

static void sched_idle_task(void) {
  while (1) {
    hal_cpu_halt();
  }
}

void sched_init(void) {
  g_free_thread_head = 0U;
  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
    g_threads[i].in_use = 0U;
    g_threads[i].next_free = (i + 1U < BHARAT_ARRAY_SIZE(g_threads)) ? (uint32_t)(i + 1U) : UINT32_MAX;
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

  kprocess_t *idle_process = process_create("idle_process");
  for (uint32_t core = 0; core < MAX_SUPPORTED_CORES; ++core) {
    g_runqueues[core].current_thread = NULL;
    g_runqueues[core].idle_thread = NULL;
    g_runqueues[core].total_ticks = 0U;
    g_runqueues[core].context_switches = 0U;
    g_runqueues[core].throttled = 0U;
    list_init(&g_runqueues[core].sleeping_list);
    list_init(&g_runqueues[core].blocked_list);
    for (uint32_t p = 0; p < MAX_PRIORITY_LEVELS; ++p) {
      list_init(&g_runqueues[core].ready_queue[p]);
    }

    kthread_t *idle = thread_create(idle_process, sched_idle_task);
    if (idle) {
      idle->priority = 0U;
      idle->base_priority = 0U;
      idle->bound_core_id = core;
      idle->affinity_mask = (1U << core);
      g_runqueues[core].idle_thread = idle;
    }
  }
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

kthread_t *thread_create(kprocess_t *parent, void (*entry_point)(void)) {
  thread_slot_t *slot = sched_find_free_thread_slot();
  if (!slot) {
    return NULL;
  }

  slot->in_use = 1U;
  slot->is_on_runqueue = 0U;
  slot->is_sleeping = 0U;

  slot->thread.thread_id = g_next_thread_id++;
  slot->thread.process_id = parent ? parent->process_id : 0U;
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

  slot->context.pc = (uint64_t)(uintptr_t)entry_point;
  slot->context.sp = 0U;
  slot->thread.cpu_context = &slot->context;

  ai_sched_init_context(&slot->ai_ctx);
  slot->ai_ctx.thread_id = (uint32_t)slot->thread.thread_id;
  slot->thread.ai_sched_ctx = &slot->ai_ctx;

  list_init(&slot->run_node);
  list_init(&slot->sleep_node);

  if (parent && !parent->main_thread) {
    parent->main_thread = &slot->thread;
  }

  if (entry_point != (void (*)(void))sched_idle_task) {
    (void)sched_enqueue(&slot->thread, slot->thread.bound_core_id);
  }

  return &slot->thread;
}

int thread_destroy(kthread_t *thread) {
  if (!thread) {
    return -1;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
  if (!slot) {
    return -1;
  }

  if (slot->is_on_runqueue != 0U) {
    list_del(&slot->run_node);
    slot->is_on_runqueue = 0U;
  }
  if (slot->is_sleeping != 0U) {
    sched_sleep_dequeue(slot);
  }

  if (thread->capability_list) {
    cap_table_destroy(thread->capability_list);
    thread->capability_list = NULL;
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
  uint32_t count = 0U;
  for (uint32_t p = 0; p < MAX_PRIORITY_LEVELS; ++p) {
    list_head_t *head = &g_runqueues[core_id].ready_queue[p];
    for (list_head_t *n = head->next; n != head; n = n->next) {
      ++count;
    }
  }
  return count;
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

kthread_t *sched_pick_next_ready(uint32_t core_id) {
  core_id = sched_clamp_core(core_id);

  if (g_policy == SCHED_POLICY_ROUND_ROBIN) {
    for (int prio = 0; prio <= (int)SCHED_MAX_PRIORITY; ++prio) {
      list_head_t *head = &g_runqueues[core_id].ready_queue[prio];
      if (!list_empty(head)) {
        list_head_t *node = head->prev;
        thread_slot_t *slot = list_entry(node, thread_slot_t, run_node);
        list_del(node);
        list_init(node);
        slot->is_on_runqueue = 0U;
        return &slot->thread;
      }
    }
  } else {
    for (int prio = (int)SCHED_MAX_PRIORITY; prio >= 0; --prio) {
      list_head_t *head = &g_runqueues[core_id].ready_queue[prio];
      if (!list_empty(head)) {
        list_head_t *node = head->prev;
        thread_slot_t *slot = list_entry(node, thread_slot_t, run_node);
        list_del(node);
        list_init(node);
        slot->is_on_runqueue = 0U;
        return &slot->thread;
      }
    }
  }

  return g_runqueues[core_id].idle_thread;
}

void sched_dequeue_task_l0(kthread_t *thread, uint32_t core_id) {
  (void)core_id;
  if (!thread) {
    return;
  }
  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
  if (slot && slot->is_on_runqueue != 0U) {
    list_del(&slot->run_node);
    list_init(&slot->run_node);
    slot->is_on_runqueue = 0U;
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
    return;
  }

  kthread_t *current = g_runqueues[core_id].current_thread;
  if (current && current != g_runqueues[core_id].idle_thread &&
      current->state == THREAD_STATE_RUNNING) {
    (void)sched_enqueue(current, core_id);
  }

  next->state = THREAD_STATE_RUNNING;
  next->context_switch_count++;
  g_sched_context_switches++;
  g_runqueues[core_id].context_switches++;
  g_runqueues[core_id].current_thread = next;

  if (fv_secure_context_switch) {
    fv_secure_context_switch(next->cpu_context);
  }
}

void sched_reschedule(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  sched_process_pending_ai_suggestions();

  if (g_runqueues[core].throttled != 0U && g_runqueues[core].idle_thread) {
    sched_switch_to(g_runqueues[core].idle_thread, core);
    return;
  }

  kthread_t *next = sched_pick_next_ready(core);
  sched_switch_to(next, core);
}

void sched_yield(void) { sched_reschedule(); }

void sched_sleep(uint64_t millis) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  kthread_t *current = g_runqueues[core].current_thread;
  if (!current || current == g_runqueues[core].idle_thread) {
    return;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid(current->thread_id);
  if (!slot) {
    return;
  }

  current->wake_deadline_ms = g_sched_ticks + millis;
  current->state = THREAD_STATE_SLEEPING;
  sched_sleep_enqueue(slot, core);
  g_runqueues[core].current_thread = NULL;
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
    sched_sleep_dequeue(slot);
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
  if (!candidate || candidate == g_runqueues[busiest].idle_thread) {
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
  g_runqueues[core].total_ticks++;

  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
    if (g_threads[i].in_use != 0U && g_threads[i].thread.state == THREAD_STATE_SLEEPING &&
        g_threads[i].thread.wake_deadline_ms <= g_sched_ticks) {
      sched_wakeup(&g_threads[i].thread);
    }
  }

  sched_process_pending_ai_suggestions();

  if ((g_sched_ticks % 16U) == 0U && core == 0U) {
    sched_balance_once();
  }

  kthread_t *current = g_runqueues[core].current_thread;
  if (!current) {
    sched_reschedule();
    return;
  }

  current->cpu_time_consumed++;
  sched_update_telemetry(current);

  if (current->cpu_time_consumed >= current->time_slice_ms) {
    current->cpu_time_consumed = 0U;
    sched_reschedule();
    return;
  }

  for (int prio = (int)SCHED_MAX_PRIORITY; prio > (int)current->priority; --prio) {
    if (!list_empty(&g_runqueues[core].ready_queue[prio])) {
      sched_reschedule();
      return;
    }
  }
}

kthread_t *sched_current_thread(void) {
  return g_runqueues[sched_clamp_core(hal_cpu_get_id())].current_thread;
}

kthread_t *sched_current(void) { return sched_current_thread(); }

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
  return thread_destroy(&slot->thread);
}

int sched_sys_sleep(uint64_t millis) {
  sched_sleep(millis);
  return 0;
}

void sched_inherit_priority(kthread_t *thread, uint32_t new_priority) {
  if (!thread) {
    return;
  }
  if (new_priority > SCHED_MAX_PRIORITY) {
    new_priority = SCHED_MAX_PRIORITY;
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
    list_del(&slot->run_node);
    list_init(&slot->run_node);
    slot->is_on_runqueue = 0U;
  }

  thread->priority = new_priority;
  if (thread->state == THREAD_STATE_READY) {
    (void)sched_enqueue(thread, thread->bound_core_id);
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
  if (!thread || new_node >= MAX_SUPPORTED_CORES) {
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
    list_del(&slot->run_node);
    list_init(&slot->run_node);
    slot->is_on_runqueue = 0U;
  }

  thread->bound_core_id = new_node;
  thread->preferred_numa_node = (uint8_t)new_node;
  if (thread->state == THREAD_STATE_READY) {
    return sched_enqueue(thread, new_node);
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
    for (uint32_t core = 0; core < MAX_SUPPORTED_CORES; ++core) {
      if ((affinity_mask & (1U << core)) != 0U) {
        return sched_migrate_task(thread, core);
      }
    }
    return -1;
  }
  return 0;
}

int sched_throttle_core(uint32_t core_id) {
  if (core_id >= MAX_SUPPORTED_CORES) {
    return -1;
  }
  g_runqueues[core_id].throttled = 1U;
  return 0;
}

int sched_ai_apply_suggestion(const ai_suggestion_t *suggestion) {
  if (!suggestion) {
    return -1;
  }
  kthread_t *thread = sched_find_thread_by_id((uint64_t)suggestion->target_id);
  switch (suggestion->action) {
  case AI_ACTION_ADJUST_PRIORITY:
    return sched_adjust_priority(thread, suggestion->value);
  case AI_ACTION_MIGRATE_TASK:
    return sched_migrate_task(thread, suggestion->value);
  case AI_ACTION_THROTTLE_CORE:
    return sched_throttle_core(suggestion->value);
  case AI_ACTION_KILL_TASK:
    return thread ? thread_destroy(thread) : -1;
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
