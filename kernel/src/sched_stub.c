#include "sched.h"
#include "kernel_safety.h"
#include "capability.h"

#include <stddef.h>
#include <stdint.h>

#define SCHED_MAX_THREADS 64U
#define SCHED_MAX_PROCESSES 32U
#define SCHED_DEFAULT_SLICE_MS 10U

typedef struct {
    uint8_t in_use;
    kthread_t thread;
    cpu_context_t context;
} thread_slot_t;

typedef struct {
    uint8_t in_use;
    kprocess_t process;
} process_slot_t;

static thread_slot_t g_threads[SCHED_MAX_THREADS];
static process_slot_t g_processes[SCHED_MAX_PROCESSES];

static kthread_t* g_current;
static sched_policy_t g_policy = SCHED_POLICY_ROUND_ROBIN;
static uint64_t g_next_thread_id = 1U;
static uint64_t g_next_process_id = 1U;

void fv_secure_context_switch(void* next_thread_frame) __attribute__((weak));

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

        if (!best || g_threads[idx].thread.priority > best->priority) {
            best = &g_threads[idx].thread;
        }
    }

    return best;
}

static void sched_switch_to(kthread_t* next) {
    if (!next) {
        return;
    }

    if (g_current && g_current->state == THREAD_STATE_RUNNING) {
        g_current->state = THREAD_STATE_READY;
    }

    next->state = THREAD_STATE_RUNNING;
    g_current = next;

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

    g_current = NULL;
    g_next_thread_id = 1U;
    g_next_process_id = 1U;
    g_policy = SCHED_POLICY_ROUND_ROBIN;
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
    if (!parent || !entry_point) {
        return NULL;
    }

    thread_slot_t* slot = sched_find_free_thread_slot();
    if (!slot) {
        return NULL;
    }

    slot->in_use = 1U;
    slot->thread.thread_id = g_next_thread_id++;
    slot->thread.process_id = parent->process_id;
    slot->thread.cpu_context = &slot->context;
    slot->thread.kernel_stack = 0U;
    slot->thread.state = THREAD_STATE_READY;
    slot->thread.priority = 1U;
    slot->thread.base_priority = 1U;
    slot->thread.waiting_on_lock = NULL;
    slot->thread.capability_list = NULL;
    slot->thread.time_slice_ms = SCHED_DEFAULT_SLICE_MS;
    slot->thread.cpu_time_consumed = 0U;
    slot->thread.preferred_numa_node = 0U;

    slot->context.pc = (uint64_t)(uintptr_t)entry_point;
    slot->context.sp = 0U;

    if (!parent->main_thread) {
        parent->main_thread = &slot->thread;
    }

    return &slot->thread;
}

int thread_destroy(kthread_t* thread) {
    if (!thread) {
        return -1;
    }

    thread_slot_t* slot = sched_find_thread_slot_by_tid(thread->thread_id);
    if (!slot) {
        return -2;
    }

    slot->thread.state = THREAD_STATE_TERMINATED;
    if (g_current == &slot->thread) {
        g_current = NULL;
    }
    slot->in_use = 0U;
    return 0;
}

void sched_yield(void) {
    kthread_t* next = sched_pick_next_ready();
    if (next) {
        sched_switch_to(next);
    }
}

void sched_on_timer_tick(void) {
    if (g_current) {
        g_current->cpu_time_consumed++;
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
    (void)g_policy; // current implementation keeps RR semantics for both profiles.
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

int sched_set_thread_priority(uint64_t tid, uint32_t new_priority) {
    kthread_t* thread = sched_find_thread_by_id(tid);
    if (!thread) {
        return -1;
    }

    if (new_priority > SCHED_MAX_PRIORITY) {
        new_priority = SCHED_MAX_PRIORITY;
    }

    thread->priority = new_priority;
    return 0;
}

int sched_set_thread_preferred_node(uint64_t tid, uint8_t node_id) {
    kthread_t* thread = sched_find_thread_by_id(tid);
    if (!thread) {
        return -1;
    }

    thread->preferred_numa_node = node_id;
    return 0;
}


int sched_ai_apply_suggestion(const ai_suggestion_t* suggestion) {
    if (!suggestion) {
        return -1;
    }

    switch (suggestion->action) {
        case AI_ACTION_ADJUST_PRIORITY:
            return sched_set_thread_priority((uint64_t)suggestion->target_id, suggestion->value);
        case AI_ACTION_MIGRATE_TASK:
            return sched_set_thread_preferred_node((uint64_t)suggestion->target_id, (uint8_t)suggestion->value);
        case AI_ACTION_THROTTLE_CORE:
            return 0;
        case AI_ACTION_NONE:
        default:
            return -1;
    }
}

#ifdef Profile_RTOS
void sched_disable_tick_for_core(uint32_t core_id) {
    (void)core_id;
}
#endif
