#include "sched.h"
#include "kernel_safety.h"
#include "capability.h"
#include "slab.h"
#include "numa.h"
#include "advanced/formal_verif.h"
#include "hal/hal.h"
#include "advanced/algo_matrix.h"
#include "../include/ipc_async.h"

#include <stddef.h>
#include <stdint.h>

void sched_enqueue_task(kthread_t* thread, uint32_t core_id);
void sched_dequeue_task(kthread_t* thread, uint32_t core_id);
void sched_enqueue_task_l0(kthread_t* thread, uint32_t core_id);
void sched_dequeue_task_l0(kthread_t* thread, uint32_t core_id);

#define MAX_SUPPORTED_CORES 8U
#define SCHED_MAX_THREADS 128U
#define SCHED_MAX_PROCESSES 32U
#define SCHED_DEFAULT_SLICE_MS 10U
#define MAX_PRIORITY_LEVELS (SCHED_MAX_PRIORITY + 1)
#define SCHED_MAX_PENDING_SUGGESTIONS 64U

typedef struct {
    uint8_t in_use;
    kthread_t thread;
    cpu_context_t context;
    ai_sched_context_t ai_ctx;
    list_head_t list_node;
    int16_t tid_hash_next;
} thread_slot_t;

typedef struct {
    uint8_t in_use;
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
    uint32_t active_priority_mask;
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

static kcache_t* thread_cache = NULL;

static int16_t g_tid_hash_heads[256];

static uint8_t sched_tid_hash(uint64_t tid) {
    // Simple mixing hash
    tid ^= tid >> 32;
    tid ^= tid >> 16;
    tid ^= tid >> 8;
    return (uint8_t)(tid & 0xFF);
}

void fv_secure_context_switch(void* next_thread_frame) __attribute__((weak));
uint32_t numa_active_node_count(void) __attribute__((weak));

static uint32_t sched_numa_node_count(void) __attribute__((unused));
static uint32_t sched_numa_node_count(void) {
    if (numa_active_node_count) {
        uint32_t count = numa_active_node_count();
        return (count == 0U) ? 1U : count;
    }
    return 1U;
}

static thread_slot_t* sched_find_thread_slot_by_tid(uint64_t tid) {
    uint8_t hash = sched_tid_hash(tid);
    int16_t current = g_tid_hash_heads[hash];

    while (current != -1) {
        if (current >= 0 && current < (int16_t)BHARAT_ARRAY_SIZE(g_threads)) {
            if (g_threads[current].in_use != 0U && g_threads[current].thread.thread_id == tid) {
                return &g_threads[current];
            }
            current = g_threads[current].tid_hash_next;
        } else {
            break;
        }
    }
    return NULL;
}

static thread_slot_t* sched_find_free_thread_slot(void) {
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
        if (g_threads[i].in_use == 0U) {
            return &g_threads[i];
        }
    }
    return NULL;
}

static process_slot_t* sched_find_free_process_slot(void) {
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_processes); ++i) {
        if (g_processes[i].in_use == 0U) {
            return &g_processes[i];
        }
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
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
        g_threads[i].in_use = 0U;
        g_threads[i].tid_hash_next = -1;
    }
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_processes); ++i) {
        g_processes[i].in_use = 0U;
    }
    for (size_t i = 0; i < 256; ++i) {
        g_tid_hash_heads[i] = -1;
    }

    g_next_thread_id = 1U;
    g_next_process_id = 1U;
    g_policy = SCHED_POLICY_ROUND_ROBIN;
    g_sched_ticks = 0U;
    g_sched_context_switches = 0U;
    g_pending_suggestions.head = 0U;
    g_pending_suggestions.tail = 0U;

    for (uint32_t i = 0; i < MAX_SUPPORTED_CORES; ++i) {
        g_runqueues[i].current_thread = NULL;
        for (uint32_t p = 0; p < MAX_PRIORITY_LEVELS; ++p) {
            list_init(&g_runqueues[i].ready_queue[p]);
        }
        g_runqueues[i].active_weight = 0U;
        g_runqueues[i].active_priority_mask = 0U;
        g_runqueues[i].total_ticks = 0U;
        g_runqueues[i].throttled = 0U;
    }

    kprocess_t* idle_process = process_create("idle_process");

    for (uint32_t i = 0; i < MAX_SUPPORTED_CORES; ++i) {
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
        return NULL;
    }

    if (cap_table_init_for_process(&slot->process) != 0) {
        slot->in_use = 0U;
        return NULL;
    }

    return &slot->process;
}

kthread_t* thread_create(kprocess_t* parent, void (*entry_point)(void)) {
    if (!thread_cache) {
        thread_cache = kcache_create("kthread_t", sizeof(kthread_t));
    }
    kthread_t* t = (kthread_t*)kcache_alloc(thread_cache);
    if (!t) return NULL;

    t->thread_id = g_next_thread_id++;
    t->process_id = parent ? parent->process_id : 0;
    t->state = THREAD_STATE_READY;
    t->priority = 1U;
    t->base_priority = 1U;
    t->cpu_time_consumed = 0U;
    t->time_slice_ms = SCHED_DEFAULT_SLICE_MS;
    t->mm_color_policy.policy = MM_COLOR_POLICY_NONE;
    t->mm_color_policy.domain = MM_DOMAIN_DEFAULT;
    t->mm_color_policy.color_mask = 0xFFFFFFFF; // all colors allowed by default
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

        uint8_t hash = sched_tid_hash(t->thread_id);
        int16_t index = (int16_t)(slot - g_threads);
        slot->tid_hash_next = g_tid_hash_heads[hash];
        g_tid_hash_heads[hash] = index;

        // Add to the local runqueue
        uint32_t core = slot->thread.bound_core_id;
        if (g_sched_ops.enqueue_task) {
            g_sched_ops.enqueue_task(&slot->thread, core);
        } else {
            sched_enqueue_task_l0(&slot->thread, core);
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
        if (g_sched_ops.dequeue_task) {
            g_sched_ops.dequeue_task(&slot->thread, slot->thread.bound_core_id);
        } else {
            sched_dequeue_task_l0(&slot->thread, slot->thread.bound_core_id);
        }

        uint8_t hash = sched_tid_hash(thread->thread_id);
        int16_t current = g_tid_hash_heads[hash];
        int16_t prev = -1;
        int16_t target = (int16_t)(slot - g_threads);

        while (current != -1) {
            if (current == target) {
                if (prev == -1) {
                    g_tid_hash_heads[hash] = slot->tid_hash_next;
                } else {
                    g_threads[prev].tid_hash_next = slot->tid_hash_next;
                }
                break;
            }
            prev = current;
            current = g_threads[current].tid_hash_next;
        }

        slot->tid_hash_next = -1;
        slot->in_use = 0;
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

static uint32_t sched_clamp_time_slice(uint64_t time_slice_ms) {
    if (time_slice_ms < 2U) {
        return 2U;
    }
    if (time_slice_ms > 20U) {
        return 20U;
    }
    return (uint32_t)time_slice_ms;
}

static int sched_highest_active_priority(uint32_t core_id) {
    uint32_t mask = g_runqueues[core_id].active_priority_mask;
    if (mask == 0U) {
        return -1;
    }
    return (int)(31U - (uint32_t)__builtin_clz(mask));
}

static void sched_update_telemetry(kthread_t* thread) {
    if (!thread || !thread->ai_sched_ctx) return;
    uint32_t core_id = hal_cpu_get_id();
    ai_sched_collect_sample(thread->ai_sched_ctx,
                            thread->time_slice_ms,
                            thread->cpu_time_consumed,
                            sched_run_queue_depth(core_id),
                            (uint32_t)thread->context_switch_count);

    if (thread->state == THREAD_STATE_RUNNING) {
        switch (thread->ai_sched_ctx->predicted_complexity) {
            case 2U:
                thread->time_slice_ms = sched_clamp_time_slice(thread->time_slice_ms + 1U);
                break;
            case 0U:
                if (thread->time_slice_ms > 2U) {
                    thread->time_slice_ms = sched_clamp_time_slice(thread->time_slice_ms - 1U);
                }
                break;
            default:
                break;
        }
    }
}

// Level 0: Reference generic O(N) iterative run-queue lookup
kthread_t* sched_pick_next_ready_l0(uint32_t core_id) {
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

// Level 1: Optimized SMP bitmask lookup (O(1) scheduling simulation for PoC)
// We would typically track a bitmask of active priorities:
// int p = 31 - __builtin_clz(runqueue->active_priority_mask);
// For this PoC, we will implement a slightly optimized loop or simulation.
kthread_t* sched_pick_next_ready_l1(uint32_t core_id) {
    while (1) {
        int p = sched_highest_active_priority(core_id);
        if (p < 0) {
            return g_runqueues[core_id].idle_thread;
        }

        list_head_t* queue = &g_runqueues[core_id].ready_queue[(uint32_t)p];
        if (list_empty(queue)) {
            g_runqueues[core_id].active_priority_mask &= ~(1U << (uint32_t)p);
            continue;
        }

        list_head_t* node = queue->next;
        thread_slot_t* slot = list_entry(node, thread_slot_t, list_node);
        list_del(node);
        list_init(node);

        if (list_empty(queue)) {
            g_runqueues[core_id].active_priority_mask &= ~(1U << (uint32_t)p);
        }
        return &slot->thread;
    }
}

kthread_t* sched_pick_next_ready(uint32_t core_id) {
    if (g_sched_ops.pick_next_ready) {
        return g_sched_ops.pick_next_ready(core_id);
    }
    // Fallback if matrix not initialized
    return sched_pick_next_ready_l0(core_id);
}

// Level 0 Enqueue
void sched_enqueue_task_l0(kthread_t* thread, uint32_t core_id) {
    if (!thread) return;
    thread_slot_t* slot = sched_find_thread_slot_by_tid(thread->thread_id);
    if (slot && thread->priority < MAX_PRIORITY_LEVELS) {
        list_add(&slot->list_node, &g_runqueues[core_id].ready_queue[thread->priority]);
        g_runqueues[core_id].active_priority_mask |= (1U << thread->priority);
    }
}

// Level 1 Enqueue (e.g. updating the active bitmap)
void sched_enqueue_task_l1(kthread_t* thread, uint32_t core_id) {
    if (!thread) return;
    thread_slot_t* slot = sched_find_thread_slot_by_tid(thread->thread_id);
    if (slot && thread->priority < MAX_PRIORITY_LEVELS) {
        list_add(&slot->list_node, &g_runqueues[core_id].ready_queue[thread->priority]);
        g_runqueues[core_id].active_priority_mask |= (1U << thread->priority);
    }
}

void sched_enqueue_task(kthread_t* thread, uint32_t core_id) {
    if (g_sched_ops.enqueue_task) {
        g_sched_ops.enqueue_task(thread, core_id);
    } else {
        sched_enqueue_task_l0(thread, core_id);
    }
}

// Level 0 Dequeue
void sched_dequeue_task_l0(kthread_t* thread, uint32_t core_id) {
    if (!thread) return;
    thread_slot_t* slot = sched_find_thread_slot_by_tid(thread->thread_id);
    if (slot && !list_empty(&slot->list_node)) {
        uint32_t priority = thread->priority;
        list_del(&slot->list_node);
        list_init(&slot->list_node);
        if (priority < MAX_PRIORITY_LEVELS && list_empty(&g_runqueues[core_id].ready_queue[priority])) {
            g_runqueues[core_id].active_priority_mask &= ~(1U << priority);
        }
    }
}

// Level 1 Dequeue (e.g. updating the active bitmap)
void sched_dequeue_task_l1(kthread_t* thread, uint32_t core_id) {
    if (!thread) return;
    thread_slot_t* slot = sched_find_thread_slot_by_tid(thread->thread_id);
    if (slot && !list_empty(&slot->list_node)) {
        uint32_t priority = thread->priority;
        list_del(&slot->list_node);
        list_init(&slot->list_node);
        if (priority < MAX_PRIORITY_LEVELS && list_empty(&g_runqueues[core_id].ready_queue[priority])) {
            g_runqueues[core_id].active_priority_mask &= ~(1U << priority);
        }
    }
}

void sched_dequeue_task(kthread_t* thread, uint32_t core_id) {
    if (g_sched_ops.dequeue_task) {
        g_sched_ops.dequeue_task(thread, core_id);
    } else {
        sched_dequeue_task_l0(thread, core_id);
    }
}

static void sched_switch_to(kthread_t* next, uint32_t core_id) {
    if (!next) return;

    kthread_t* current = g_runqueues[core_id].current_thread;
    if (current == next) {
        current->state = THREAD_STATE_RUNNING;
        return;
    }
    if (current && current->state == THREAD_STATE_RUNNING && current != g_runqueues[core_id].idle_thread) {
        current->state = THREAD_STATE_READY;
        sched_enqueue_task(current, core_id);
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

void sched_wakeup_with_priority(kthread_t* thread, uint32_t wakeup_priority) {
    if (!thread) return;

    if (wakeup_priority <= SCHED_MAX_PRIORITY && wakeup_priority > thread->priority) {
        thread->priority = wakeup_priority;
    }

    if (thread->state == THREAD_STATE_SLEEPING || thread->state == THREAD_STATE_BLOCKED) {
        thread->state = THREAD_STATE_READY;
        thread->wake_deadline_ms = 0U;
        thread_slot_t* slot = sched_find_thread_slot_by_tid(thread->thread_id);
        if (slot) {
            uint32_t core = thread->bound_core_id;
            sched_enqueue_task(thread, core);
        }
    }
}

void sched_wakeup(kthread_t* thread) {
    sched_wakeup_with_priority(thread, SCHED_MAX_PRIORITY + 1U);
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
    sched_enqueue_task(migrated, idlest_core);
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

    // Call IPC subsystem hook to handle async cancellations/timeouts
    ipc_async_check_timeouts(g_sched_ticks);

    sched_process_pending_ai_suggestions();

    if ((g_sched_ticks % 16U) == 0U && core_id == 0U) {
        sched_balance_once();
    }

    if (core_id < MAX_SUPPORTED_CORES) {
        kthread_t* current = g_runqueues[core_id].current_thread;
        if (current) {
            current->cpu_time_consumed++;
            sched_update_telemetry(current);

            if (current->cpu_time_consumed % 100 == 0) {
                // Periodically balance NUMA memory (e.g. every 100 ticks)
                // This triggers the deferred/worker evaluation logic without blocking heavily
                numa_balance_thread_memory((void*)current);
            }

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
    sched_dequeue_task(thread, thread->bound_core_id);

    thread->bound_core_id = new_node;
    thread->preferred_numa_node = new_node;
    if (thread->state == THREAD_STATE_READY) {
        sched_enqueue_task(thread, new_node);
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

// Multikernel IPC integration stub
void sched_notify_ipc_ready(uint32_t core_id, uint32_t msg_type) {
    (void)core_id;
    (void)msg_type;
    // TODO: In Phase 2/3, this hook will map incoming messages to waiting threads
    // and trigger sched_wakeup() or IPI wakeups for specific thread runqueues.
}

#ifdef Profile_RTOS
void sched_disable_tick_for_core(uint32_t core_id) {
    (void)core_id;
}
#endif
