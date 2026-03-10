
#include "sched.h"
#include "sched_deg.h"
#include "kernel_safety.h"
#include "capability.h"
#include "../include/slab.h"
#include "advanced/formal_verif.h"

#include <stddef.h>
#include <stdint.h>

#define SCHED_MAX_THREADS 64U
#define SCHED_MAX_PROCESSES 32U
#define SCHED_DEFAULT_SLICE_MS 10U
#define SCHED_MAX_CORES 8U
#define SCHED_MAX_PENDING_SUGGESTIONS 64U

typedef struct {
    uint8_t in_use;
    kthread_t thread;
    cpu_context_t context;
    ai_sched_context_t ai_ctx;
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

static thread_slot_t g_threads[SCHED_MAX_THREADS];
static process_slot_t g_processes[SCHED_MAX_PROCESSES];
static sched_core_t g_cores[SCHED_MAX_CORES];

static kthread_t* g_current;
static sched_policy_t g_policy = SCHED_POLICY_ROUND_ROBIN;
static uint64_t g_next_thread_id = 1U;
static uint64_t g_next_process_id = 1U;
static uint64_t g_sched_ticks = 0U;
static uint64_t g_sched_context_switches = 0U;
static suggestion_queue_t g_pending_suggestions;

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

static uint32_t sched_run_queue_depth(void) {
    uint32_t count = 0U;
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
        if (g_threads[i].in_use != 0U && g_threads[i].thread.state == THREAD_STATE_READY) {
            ++count;
        }
    }
    return count;
}

static int sched_suggestion_dequeue(ai_suggestion_t* out) {
    if (!out) {
        return -1;
    }

    if (g_pending_suggestions.head == g_pending_suggestions.tail) {
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

static kthread_t* sched_pick_next_ready(void) {
    size_t start = 0U;
    kthread_t* best = NULL;

    if (g_current) {
        thread_slot_t* cur = sched_find_thread_slot_by_tid(g_current->thread_id);
        if (cur) {
            start = (size_t)(cur - &g_threads[0]) + 1U;
        }
    }

    for (size_t attempt = 0U; attempt < BHARAT_ARRAY_SIZE(g_threads); ++attempt) {
        size_t idx = (start + attempt) % BHARAT_ARRAY_SIZE(g_threads);
        if (g_threads[idx].in_use == 0U || g_threads[idx].thread.state != THREAD_STATE_READY) {
            continue;
        }

        if (g_policy == SCHED_POLICY_EDF && g_threads[idx].thread.rt_attr.deadline_ms > 0U) {
            if (!best || g_threads[idx].thread.rt_attr.deadline_ms < best->rt_attr.deadline_ms) {
                best = &g_threads[idx].thread;
            }
            continue;
        }

        if (!best || g_threads[idx].thread.priority > best->priority) {
            best = &g_threads[idx].thread;
        }
    }

    return best;
}

static void sched_update_telemetry(kthread_t* thread) {
    if (!thread || !thread->ai_sched_ctx) {
        return;
    }

    ai_sched_collect_sample(thread->ai_sched_ctx,
                            thread->time_slice_ms,
                            thread->cpu_time_consumed,
                            sched_run_queue_depth(),
                            (uint32_t)thread->context_switch_count);
}

static void sched_switch_to(kthread_t* next) {
    if (!next) {
        return;
    }

    if (g_current && g_current->state == THREAD_STATE_RUNNING) {
        g_current->state = THREAD_STATE_READY;
    }

    next->state = THREAD_STATE_RUNNING;
    next->context_switch_count++;
    g_sched_context_switches++;
    g_current = next;
    g_cores[0].current = next;

    if (fv_secure_context_switch) {
        fv_secure_context_switch(next->cpu_context);
    }
}

void sched_init(void) {
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
        g_threads[i].in_use = 0U;
    }
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_processes); ++i) {
        g_processes[i].in_use = 0U;
    }
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_cores); ++i) {
        g_cores[i] = (sched_core_t){ .core_id = (uint32_t)i, .current = NULL, .total_ticks = 0U, .throttled = 0U };
    }

    g_current = NULL;
    g_next_thread_id = 1U;
    g_next_process_id = 1U;
    g_policy = SCHED_POLICY_ROUND_ROBIN;
    g_sched_ticks = 0U;
    g_sched_context_switches = 0U;
    g_pending_suggestions.head = 0U;
    g_pending_suggestions.tail = 0U;
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

    queue->head = thread->next_waiter;
    if (!queue->head) {
        queue->tail = NULL;
    }

    thread->next_waiter = NULL;
    return thread;
}

void sched_block(void) {
    if (g_current) {
        g_current->state = THREAD_STATE_BLOCKED;
        if (g_current->sched_ctx && g_current->sched_ctx->deg) {
            deg_block_member(g_current, 0);
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
    t->process_id = parent->process_id;
    t->state = THREAD_STATE_READY;
    t->priority = 1U;
    t->base_priority = 1U;
    t->cpu_time_consumed = 0U;
    t->time_slice_ms = SCHED_DEFAULT_SLICE_MS;
    t->preferred_numa_node = 0U;

    thread_slot_t* slot = sched_find_free_thread_slot();
    if (slot) {
        slot->in_use = 1U;
        slot->thread = *t;
        slot->context.pc = (uint64_t)(uintptr_t)entry_point;
        slot->context.sp = 0U;
        slot->thread.cpu_context = &slot->context;

        if (!parent->main_thread) {
            parent->main_thread = &slot->thread;
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
        slot->in_use = 0;
    }
    if (thread_cache) {
        kcache_free(thread_cache, thread);
    }
    return 0;
}

void sched_yield(void) {
    sched_process_pending_ai_suggestions();
    kthread_t* next = sched_pick_next_ready();
    if (next) {
        sched_switch_to(next);
    }
}

void sched_sleep(uint64_t millis) {
    if (!g_current) {
        return;
    }

    g_current->wake_deadline_ms = g_sched_ticks + millis;
    g_current->state = THREAD_STATE_SLEEPING;
    sched_yield();
}

void sched_wakeup(kthread_t* thread) {
    if (!thread) {
        return;
    }

    if (thread->state == THREAD_STATE_SLEEPING || thread->state == THREAD_STATE_BLOCKED) {
        thread->state = THREAD_STATE_READY;
        thread->wake_deadline_ms = 0U;
    }
}

void sched_on_timer_tick(void) {
    g_sched_ticks++;
    g_cores[0].total_ticks++;

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
        if (g_threads[i].in_use != 0U && g_threads[i].thread.state == THREAD_STATE_SLEEPING &&
            g_threads[i].thread.wake_deadline_ms <= g_sched_ticks) {
            sched_wakeup(&g_threads[i].thread);
        }
    }

    sched_process_pending_ai_suggestions();

    if (g_current) {
        g_current->cpu_time_consumed++;
        sched_update_telemetry(g_current);

        if (g_current->cpu_time_consumed >= g_current->time_slice_ms) {
            g_current->cpu_time_consumed = 0U;
            sched_yield();
        }
        return;
    }

    sched_yield();
}

kthread_t* sched_current_thread(void) {
    return g_current;
}

void sched_set_policy(sched_policy_t policy) {
    g_policy = policy;
}

int sched_sys_thread_create(kprocess_t* parent, void (*entry_point)(void), uint64_t* out_tid) {
    kthread_t* t = thread_create(parent, entry_point);
    if (!t) {
        return -1;
    }

    if (out_tid) {
        *out_tid = t->thread_id;
    }
    return 0;
}

int sched_sys_thread_destroy(uint64_t tid) {
    thread_slot_t* slot = sched_find_thread_slot_by_tid(tid);
    if (!slot) {
        return -1;
    }

    return thread_destroy(&slot->thread);
}

void sched_inherit_priority(kthread_t* thread, uint32_t new_priority) {
    if (!thread) {
        return;
    }

    if (new_priority > thread->priority) {
        thread->priority = new_priority;
    }
}

void sched_restore_priority(kthread_t* thread) {
    if (!thread) {
        return;
    }

    thread->priority = thread->base_priority;
}

kthread_t* sched_find_thread_by_id(uint64_t tid) {
    thread_slot_t* slot = sched_find_thread_slot_by_tid(tid);
    return slot ? &slot->thread : NULL;
}

int sched_adjust_priority(kthread_t* thread, uint32_t new_priority) {
    if (!thread) {
        return -1;
    }

    if (new_priority > SCHED_MAX_PRIORITY) {
        new_priority = SCHED_MAX_PRIORITY;
    }

    thread->priority = new_priority;

    // Also update the priority in the thread_slot_t so that the change persists
    // and is actually applied and visible in the run queue iteration.
    thread_slot_t* slot = sched_find_thread_slot_by_tid(thread->thread_id);
    if (slot) {
        slot->thread.priority = new_priority;
    }

    return 0;
}

int sched_set_thread_priority(uint64_t tid, uint32_t new_priority) {
    return sched_adjust_priority(sched_find_thread_by_id(tid), new_priority);
}

int sched_migrate_task(kthread_t* thread, uint32_t new_node) {
    if (!thread) {
        return -1;
    }

    if (new_node >= sched_numa_node_count()) {
        return -2;
    }

    thread->preferred_numa_node = (uint8_t)new_node;
    return 0;
}

int sched_set_thread_preferred_node(uint64_t tid, uint8_t node_id) {
    return sched_migrate_task(sched_find_thread_by_id(tid), node_id);
}

int sched_throttle_core(uint32_t core_id) {
    if (core_id >= SCHED_MAX_CORES) {
        return -1;
    }

    g_cores[core_id].throttled = 1U;
    return 0;
}

int sched_ai_apply_suggestion(const ai_suggestion_t* suggestion) {
    if (!suggestion) {
        return -1;
    }

    kthread_t* thread = sched_find_thread_by_id((uint64_t)suggestion->target_id);
    switch (suggestion->action) {
        case AI_ACTION_ADJUST_PRIORITY:
            return sched_adjust_priority(thread, suggestion->value);
        case AI_ACTION_MIGRATE_TASK:
            return sched_migrate_task(thread, suggestion->value);
        case AI_ACTION_THROTTLE_CORE:
            return 0; // Throttle core not implemented fully in stub
        case AI_ACTION_KILL_TASK: {
            kthread_t* thread = sched_find_thread_by_id((uint64_t)suggestion->target_id);
            if (thread) {
                return thread_destroy(thread);
            }
            return -1;
        }
        case AI_ACTION_NONE:
        default:
            return -1;
    }
}

int sched_enqueue_ai_suggestion(const ai_suggestion_t* suggestion) {
    if (!suggestion) {
        return -1;
    }

    uint32_t next = (g_pending_suggestions.head + 1U) % SCHED_MAX_PENDING_SUGGESTIONS;
    if (next == g_pending_suggestions.tail) {
        return -2;
    }

    g_pending_suggestions.queue[g_pending_suggestions.head] = *suggestion;
    g_pending_suggestions.head = next;

    if ((suggestion->action == AI_ACTION_ADJUST_PRIORITY ||
         suggestion->action == AI_ACTION_MIGRATE_TASK) &&
        g_current &&
        g_current->thread_id == suggestion->target_id) {
        sched_yield();
    }

    return 0;
}

#ifdef Profile_RTOS
void sched_disable_tick_for_core(uint32_t core_id) {
    (void)core_id;
}
#endif

int sched_enqueue(kthread_t *thread, uint32_t core_id) {
    (void)core_id;
    if (!thread) {
        return -1;
    }

    // Minimal stub for DEG enqueue support in tests
    if (thread->sched_ctx && thread->sched_ctx->deg &&
        (thread->sched_ctx->flags & SCHED_CTX_FLAG_STRICT_COSCHED)) {

        if (thread->state != THREAD_STATE_DEG_PENDING && thread->state != THREAD_STATE_READY) {
            thread->state = THREAD_STATE_DEG_PENDING;
            deg_mark_ready(thread);

            if (thread->state == THREAD_STATE_DEG_PENDING) {
                return 0; // Not yet released
            }
        }
    }

    thread->state = THREAD_STATE_READY;
    return 0;
}
