#include "sched.h"
#include "kernel_safety.h"
#include "capability.h"
#include "slab.h"
#include "advanced/formal_verif.h"
#include "hal/hal.h"
#include "advanced/algo_matrix.h"

#include <stddef.h>
#include <stdint.h>

#define MAX_SUPPORTED_CORES 8U
#define SCHED_MAX_THREADS 128U
#define SCHED_MAX_PROCESSES 32U
#define SCHED_DEFAULT_SLICE_MS 10U
#define MAX_PRIORITY_LEVELS (SCHED_MAX_PRIORITY + 1)
#define SCHED_MAX_PENDING_SUGGESTIONS 64U

typedef struct {
    uint8_t in_use;
    uint32_t next_free;
    kthread_t thread;
    cpu_context_t context;
    ai_sched_context_t ai_ctx;
    list_head_t list_node;
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

// Sample structure for per-core queues
typedef struct {
    // spinlock_t lock; // Omitting explicit spinlock implementation for simplicity, assuming IRQ disable suffices in UP or atomic ops in SMP
    kthread_t* current_thread;
    kthread_t* idle_thread;
    list_head_t ready_queue[MAX_PRIORITY_LEVELS];
    uint32_t active_weight;
    uint64_t total_ticks;
    uint32_t throttled;
} core_runqueue_t;

// Per-core array
static core_runqueue_t g_runqueues[MAX_SUPPORTED_CORES];

static thread_slot_t g_threads[SCHED_MAX_THREADS];
static process_slot_t g_processes[SCHED_MAX_PROCESSES];

static sched_policy_t g_policy = SCHED_POLICY_ROUND_ROBIN;
static uint64_t g_next_thread_id = 1U;
static uint64_t g_next_process_id = 1U;
static uint64_t g_sched_ticks = 0U;
static uint64_t g_sched_context_switches = 0U;
static suggestion_queue_t g_pending_suggestions;

static uint32_t g_free_thread_head = UINT32_MAX;
static uint32_t g_free_process_head = UINT32_MAX;

static kcache_t* thread_cache = NULL;

void fv_secure_context_switch(void* next_thread_frame) __attribute__((weak));
uint32_t numa_active_node_count(void) __attribute__((weak));

static uint32_t sched_numa_node_count(void) {
    if (numa_active_node_count) {
        uint32_t count = numa_active_node_count();
        return (count == 0U) ? 1U : count;
    }
    return 1U;
}

static thread_slot_t* sched_find_thread_slot_by_tid(uint64_t tid) {
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
        if (g_threads[i].in_use != 0U && g_threads[i].thread.thread_id == tid) {
            return &g_threads[i];
        }
    }
    return NULL;
}

static thread_slot_t* sched_find_free_thread_slot(void) {
    if (g_free_thread_head != UINT32_MAX) {
        uint32_t index = g_free_thread_head;
        g_free_thread_head = g_threads[index].next_free;
        return &g_threads[index];
    }
    return NULL;
}

static process_slot_t* sched_find_free_process_slot(void) {
    if (g_free_process_head != UINT32_MAX) {
        uint32_t index = g_free_process_head;
        g_free_process_head = g_processes[index].next_free;
        return &g_processes[index];
    }
    return NULL;
}

static void sched_idle_task(void) {
    while (1) {
        hal_cpu_halt();
        sched_yield();
    }
}

void sched_init(void) {
    g_free_thread_head = 0;
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
        g_threads[i].in_use = 0U;
        g_threads[i].next_free = (i + 1 < BHARAT_ARRAY_SIZE(g_threads)) ? (uint32_t)(i + 1) : UINT32_MAX;
    }
    g_free_process_head = 0;
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_processes); ++i) {
        g_processes[i].in_use = 0U;
        g_processes[i].next_free = (i + 1 < BHARAT_ARRAY_SIZE(g_processes)) ? (uint32_t)(i + 1) : UINT32_MAX;
    }

    g_next_thread_id = 1U;
    g_next_process_id = 1U;
    g_policy = SCHED_POLICY_ROUND_ROBIN;
    g_sched_ticks = 0U;
    g_sched_context_switches = 0U;
    g_pending_suggestions.head = 0U;
    g_pending_suggestions.tail = 0U;

    kprocess_t* idle_process = process_create("idle_process");

    for (uint32_t i = 0; i < MAX_SUPPORTED_CORES; ++i) {
        g_runqueues[i].current_thread = NULL;
        for (uint32_t p = 0; p < MAX_PRIORITY_LEVELS; ++p) {
            list_init(&g_runqueues[i].ready_queue[p]);
        }
        g_runqueues[i].active_weight = 0U;
        g_runqueues[i].total_ticks = 0U;
        g_runqueues[i].throttled = 0U;

        kthread_t* idle_th = thread_create(idle_process, sched_idle_task);
        if (idle_th) {
            idle_th->priority = 0;
            idle_th->bound_core_id = i;
            g_runqueues[i].idle_thread = idle_th;
        }
    }
}

kprocess_t* process_create(const char* name) {
    (void)name;

    process_slot_t* slot = sched_find_free_process_slot();
    if (!slot) {
        return NULL;
    }

    slot->in_use = 1U;
    slot->process.process_id = g_next_process_id++;
    slot->process.addr_space = mm_create_address_space();
    slot->process.main_thread = NULL;
    slot->process.security_sandbox_ctx = NULL;

    if (!slot->process.addr_space) {
        slot->in_use = 0U;

        uint32_t slot_index = (uint32_t)(slot - g_processes);
        slot->next_free = g_free_process_head;
        g_free_process_head = slot_index;

        return NULL;
    }

    if (cap_table_init_for_process(&slot->process) != 0) {
        slot->in_use = 0U;

        uint32_t slot_index = (uint32_t)(slot - g_processes);
        slot->next_free = g_free_process_head;
        g_free_process_head = slot_index;

        return NULL;
    }

    return &slot->process;
}

int process_destroy(kprocess_t* process) {
    if (!process) return -1;

    process_slot_t* slot = NULL;
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_processes); ++i) {
        if (g_processes[i].in_use != 0U && &g_processes[i].process == process) {
            slot = &g_processes[i];
            break;
        }
    }
    if (!slot) return -1;

    if (slot->process.security_sandbox_ctx) {
        cap_table_destroy(slot->process.security_sandbox_ctx);
        slot->process.security_sandbox_ctx = NULL;
    }

    slot->in_use = 0;

    uint32_t slot_index = (uint32_t)(slot - g_processes);
    slot->next_free = g_free_process_head;
    g_free_process_head = slot_index;

    return 0;
}

kthread_t* thread_create(kprocess_t* parent, void (*entry_point)(void)) {
    if (!thread_cache) {
        thread_cache = kcache_create("kthread_t", sizeof(kthread_t));
    }
    kthread_t* t = (kthread_t*)kcache_alloc(thread_cache);
    if (!t) return NULL;

    t->thread_id = g_next_thread_id++;
    t->process_id = parent ? parent->process_id : 0;
    t->personality = PERSONALITY_NATIVE;
    t->state = THREAD_STATE_READY;
    t->priority = 1U;
    t->base_priority = 1U;
    t->cpu_time_consumed = 0U;
    t->time_slice_ms = SCHED_DEFAULT_SLICE_MS;
    t->preferred_numa_node = 0U;
    t->bound_core_id = hal_cpu_get_id(); // Default to current core
    if (t->bound_core_id >= MAX_SUPPORTED_CORES) {
        t->bound_core_id = 0;
    }

    thread_slot_t* slot = sched_find_free_thread_slot();
    if (slot) {
        slot->in_use = 1U;
        slot->thread = *t;
        slot->context.pc = (uint64_t)(uintptr_t)entry_point;
        slot->context.sp = 0U;
        slot->thread.cpu_context = &slot->context;
        list_init(&slot->list_node);

        if (parent && !parent->main_thread) {
            parent->main_thread = &slot->thread;
        }

        // Add to the local runqueue
        uint32_t core = slot->thread.bound_core_id;
        // Do not add to runqueue directly here since runqueues might not be initialized yet
        // if this is the idle thread being created in sched_init.
        // It's the caller's responsibility or handled via explicit enqueue.
        if (entry_point != sched_idle_task && slot->thread.priority < MAX_PRIORITY_LEVELS) {
            list_add(&slot->list_node, &g_runqueues[core].ready_queue[slot->thread.priority]);
        }

        return &slot->thread;
    }

    kcache_free(thread_cache, t);
    return NULL;
}

int thread_destroy(kthread_t* thread) {
    if (!thread) return -1;
    thread_slot_t* slot = sched_find_thread_slot_by_tid(thread->thread_id);
    if (slot) {
        if (!list_empty(&slot->list_node)) {
            list_del(&slot->list_node);
        }
        slot->in_use = 0;

        uint32_t slot_index = (uint32_t)(slot - g_threads);
        slot->next_free = g_free_thread_head;
        g_free_thread_head = slot_index;
    }
    if (thread_cache) {
        kcache_free(thread_cache, thread);
    }
    return 0;
}

static int sched_suggestion_dequeue(ai_suggestion_t* out) {
    if (!out) return -1;
    if (g_pending_suggestions.head == g_pending_suggestions.tail) return -1;
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
        list_head_t* head = &g_runqueues[core_id].ready_queue[p];
        list_head_t* curr = head->next;
        while (curr != head) {
            count++;
            curr = curr->next;
        }
    }
    return count;
}

static void sched_update_telemetry(kthread_t* thread) {
    if (!thread || !thread->ai_sched_ctx) return;
    uint32_t core_id = hal_cpu_get_id();
    ai_sched_collect_sample(thread->ai_sched_ctx,
                            thread->time_slice_ms,
                            thread->cpu_time_consumed,
                            sched_run_queue_depth(core_id),
                            (uint32_t)thread->context_switch_count);
}

kthread_t* sched_pick_next_ready(uint32_t core_id) {
    for (int p = MAX_PRIORITY_LEVELS - 1; p >= 0; --p) {
        if (!list_empty(&g_runqueues[core_id].ready_queue[p])) {
            list_head_t* node = g_runqueues[core_id].ready_queue[p].next;
            thread_slot_t* slot = list_entry(node, thread_slot_t, list_node);
            list_del(node); // Dequeue
            list_init(node);
            return &slot->thread;
        }
    }
    return g_runqueues[core_id].idle_thread;
}

static void sched_switch_to(kthread_t* next, uint32_t core_id) {
    if (!next) return;

    kthread_t* current = g_runqueues[core_id].current_thread;
    if (current && current->state == THREAD_STATE_RUNNING && current != g_runqueues[core_id].idle_thread) {
        current->state = THREAD_STATE_READY;
        thread_slot_t* slot = sched_find_thread_slot_by_tid(current->thread_id);
        if (slot) {
            list_add(&slot->list_node, &g_runqueues[core_id].ready_queue[current->priority]);
        }
    } else if (current && current == g_runqueues[core_id].idle_thread) {
        current->state = THREAD_STATE_READY;
    }

    next->state = THREAD_STATE_RUNNING;
    next->context_switch_count++;
    g_sched_context_switches++;
    g_runqueues[core_id].current_thread = next;

    if (fv_secure_context_switch) {
        fv_secure_context_switch(next->cpu_context);
    }
}

void sched_yield(void) {
    uint32_t core_id = hal_cpu_get_id();
    if (core_id >= MAX_SUPPORTED_CORES) return;

    sched_process_pending_ai_suggestions();
    if (g_runqueues[core_id].throttled != 0U && g_runqueues[core_id].idle_thread) {
        sched_switch_to(g_runqueues[core_id].idle_thread, core_id);
        return;
    }

    kthread_t* next = sched_pick_next_ready(core_id);
    if (next) {
        sched_switch_to(next, core_id);
    }
}

void sched_sleep(uint64_t millis) {
    uint32_t core_id = hal_cpu_get_id();
    kthread_t* current = g_runqueues[core_id].current_thread;
    if (!current) return;

    current->wake_deadline_ms = g_sched_ticks + millis;
    current->state = THREAD_STATE_SLEEPING;
    sched_yield();
}

void sched_wakeup(kthread_t* thread) {
    if (!thread) return;

    if (thread->state == THREAD_STATE_SLEEPING) {
        thread->state = THREAD_STATE_READY;
        thread->wake_deadline_ms = 0U;
        thread_slot_t* slot = sched_find_thread_slot_by_tid(thread->thread_id);
        if (slot) {
            uint32_t core = thread->bound_core_id;
            list_add(&slot->list_node, &g_runqueues[core].ready_queue[thread->priority]);
        }
    }
}


static uint32_t sched_core_queue_depth(uint32_t core_id) {
    if (core_id >= MAX_SUPPORTED_CORES) {
        return 0U;
    }
    return sched_run_queue_depth(core_id);
}

static void sched_balance_once(void) {
    uint32_t busiest_core = 0U;
    uint32_t idlest_core = 0U;
    uint32_t max_depth = 0U;
    uint32_t min_depth = UINT32_MAX;

    for (uint32_t core = 0; core < MAX_SUPPORTED_CORES; ++core) {
        if (g_runqueues[core].throttled != 0U) {
            continue;
        }
        uint32_t depth = sched_core_queue_depth(core);
        if (depth > max_depth) {
            max_depth = depth;
            busiest_core = core;
        }
        if (depth < min_depth) {
            min_depth = depth;
            idlest_core = core;
        }
    }

    if (max_depth <= (min_depth + 1U) || busiest_core == idlest_core) {
        return;
    }

    kthread_t* migrated = sched_pick_next_ready(busiest_core);
    if (!migrated || migrated == g_runqueues[busiest_core].idle_thread) {
        return;
    }

    migrated->bound_core_id = idlest_core;
    migrated->preferred_numa_node = (uint8_t)idlest_core;
    migrated->state = THREAD_STATE_READY;
    g_sched_ops.enqueue_task(migrated, idlest_core);
}

void sched_on_timer_tick(void) {
    g_sched_ticks++;
    uint32_t core_id = hal_cpu_get_id();
    if (core_id < MAX_SUPPORTED_CORES) {
        g_runqueues[core_id].total_ticks++;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
        if (g_threads[i].in_use != 0U && g_threads[i].thread.state == THREAD_STATE_SLEEPING &&
            g_threads[i].thread.wake_deadline_ms <= g_sched_ticks) {
            sched_wakeup(&g_threads[i].thread);
        }
    }

    sched_process_pending_ai_suggestions();

    if ((g_sched_ticks % 16U) == 0U && core_id == 0U) {
        sched_balance_once();
    }

    if (core_id < MAX_SUPPORTED_CORES) {
        kthread_t* current = g_runqueues[core_id].current_thread;
        if (current) {
            current->cpu_time_consumed++;
            sched_update_telemetry(current);

            if (current->cpu_time_consumed >= current->time_slice_ms) {
                current->cpu_time_consumed = 0U;
                sched_yield();
            }
            return;
        }
    }

    sched_yield();
}

kthread_t* sched_current_thread(void) {
    uint32_t core_id = hal_cpu_get_id();
    if (core_id >= MAX_SUPPORTED_CORES) return NULL;
    return g_runqueues[core_id].current_thread;
}

uint64_t sched_get_ticks(void) {
    return g_sched_ticks;
}

void sched_set_policy(sched_policy_t policy) {
    g_policy = policy;
}

int sched_sys_thread_create(kprocess_t* parent, void (*entry_point)(void), uint64_t* out_tid) {
    kthread_t* t = thread_create(parent, entry_point);
    if (!t) return -1;
    if (out_tid) *out_tid = t->thread_id;
    return 0;
}

int sched_sys_thread_destroy(uint64_t tid) {
    thread_slot_t* slot = sched_find_thread_slot_by_tid(tid);
    if (!slot) return -1;
    return thread_destroy(&slot->thread);
}

void sched_inherit_priority(kthread_t* thread, uint32_t new_priority) {
    if (!thread) return;
    if (new_priority > thread->priority) {
        thread->priority = new_priority;
    }
}

void sched_restore_priority(kthread_t* thread) {
    if (!thread) return;
    thread->priority = thread->base_priority;
}

kthread_t* sched_find_thread_by_id(uint64_t tid) {
    thread_slot_t* slot = sched_find_thread_slot_by_tid(tid);
    return slot ? &slot->thread : NULL;
}

int sched_adjust_priority(kthread_t* thread, uint32_t new_priority) {
    if (!thread) return -1;
    if (new_priority > SCHED_MAX_PRIORITY) new_priority = SCHED_MAX_PRIORITY;
    thread->priority = new_priority;
    return 0;
}

int sched_set_thread_priority(uint64_t tid, uint32_t new_priority) {
    return sched_adjust_priority(sched_find_thread_by_id(tid), new_priority);
}

int sched_migrate_task(kthread_t* thread, uint32_t new_node) {
    if (!thread) return -1;
    if (new_node >= MAX_SUPPORTED_CORES) return -2;

    thread_slot_t* slot = sched_find_thread_slot_by_tid(thread->thread_id);
    if (!slot) return -1;

    // Remove from current queue
    if (!list_empty(&slot->list_node)) {
        list_del(&slot->list_node);
        list_init(&slot->list_node);
    }

    thread->bound_core_id = new_node;
    thread->preferred_numa_node = new_node;
    if (thread->state == THREAD_STATE_READY) {
        list_add(&slot->list_node, &g_runqueues[new_node].ready_queue[thread->priority]);
    }

    uint32_t current_core = hal_cpu_get_id();
    if (current_core < MAX_SUPPORTED_CORES && g_runqueues[current_core].current_thread == thread) {
        sched_yield(); // Forced preemption
    }

    return 0;
}

int sched_set_thread_preferred_node(uint64_t tid, uint8_t node_id) {
    // Treat NUMA node as core for the basic implementation mapping
    return sched_migrate_task(sched_find_thread_by_id(tid), node_id);
}

int sched_throttle_core(uint32_t core_id) {
    if (core_id >= MAX_SUPPORTED_CORES) return -1;
    g_runqueues[core_id].throttled = 1U;
    return 0;
}

int sched_ai_apply_suggestion(const ai_suggestion_t* suggestion) {
    if (!suggestion) return -1;
    kthread_t* thread = sched_find_thread_by_id((uint64_t)suggestion->target_id);
    switch (suggestion->action) {
        case AI_ACTION_ADJUST_PRIORITY:
            return sched_adjust_priority(thread, suggestion->value);
        case AI_ACTION_MIGRATE_TASK:
            return sched_migrate_task(thread, suggestion->value);
        case AI_ACTION_THROTTLE_CORE:
            return sched_throttle_core(suggestion->value);
        case AI_ACTION_KILL_TASK: {
            if (thread) return thread_destroy(thread);
            return -1;
        }
        case AI_ACTION_NONE:
        default:
            return -1;
    }
}

int sched_enqueue_ai_suggestion(const ai_suggestion_t* suggestion) {
    if (!suggestion) return -1;
    uint32_t next = (g_pending_suggestions.head + 1U) % SCHED_MAX_PENDING_SUGGESTIONS;
    if (next == g_pending_suggestions.tail) return -2;
    g_pending_suggestions.queue[g_pending_suggestions.head] = *suggestion;
    g_pending_suggestions.head = next;
    return 0;
}

#ifdef Profile_RTOS
void sched_disable_tick_for_core(uint32_t core_id) {
    (void)core_id;
}
#endif

// Multikernel IPC integration stub
void sched_notify_ipc_ready(uint32_t core_id, uint32_t msg_type) {
    (void)core_id;
    (void)msg_type;
}
