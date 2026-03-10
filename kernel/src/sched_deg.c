#include "sched.h"
#include "sched_deg.h"

// Define a minimal kmalloc/kfree stub if we don't have one, since this is a kernel
// Actually, I'll just use a static pool for DEGs to avoid memory allocation failures in MVP
#define MAX_SYSTEM_DEGS 32
static dist_exec_group_t g_degs[MAX_SYSTEM_DEGS];
static sched_context_t g_sched_ctxs[MAX_SYSTEM_DEGS * MAX_DEG_MEMBERS];
static uint32_t g_next_deg_idx = 0;
static uint32_t g_next_ctx_idx = 0;

dist_exec_group_t* deg_create(void) {
    if (g_next_deg_idx >= MAX_SYSTEM_DEGS) {
        return NULL;
    }

    dist_exec_group_t* deg = &g_degs[g_next_deg_idx++];

    deg->deg_id = g_next_deg_idx;
    deg->mode = DEG_STRICT;
    deg->state = DEG_WAITING_RELEASE;
    deg->member_count = 0;
    deg->ready_bitmap = 0;
    deg->running_bitmap = 0;
    deg->release_epoch = 0;
    deg->release_window_ns = 1000000; // 1 ms default slack

    for (int i = 0; i < MAX_DEG_MEMBERS; i++) {
        deg->members[i] = NULL;
    }

    return deg;
}

int deg_add_member(dist_exec_group_t* deg, struct kthread* thread, uint32_t core, uint32_t local_sched_class) {
    if (!deg || !thread || deg->member_count >= MAX_DEG_MEMBERS) {
        return -1;
    }

    if (g_next_ctx_idx >= (MAX_SYSTEM_DEGS * MAX_DEG_MEMBERS)) {
        return -1;
    }

    sched_context_t* ctx = &g_sched_ctxs[g_next_ctx_idx++];

    ctx->deg = deg;
    ctx->flags = SCHED_CTX_FLAG_STRICT_COSCHED;
    ctx->local_sched_class = local_sched_class;
    ctx->local_priority = thread->priority;
    ctx->local_affinity = core;

    thread->sched_ctx = ctx;
    thread->bound_core_id = core;

    deg->members[deg->member_count++] = thread;
    return 0;
}

int deg_activate(dist_exec_group_t* deg) {
    if (!deg) return -1;
    deg->state = DEG_WAITING_RELEASE;
    deg->ready_bitmap = 0;
    deg->running_bitmap = 0;
    return 0;
}

static int _deg_get_member_index(dist_exec_group_t* deg, struct kthread* thread) {
    for (uint32_t i = 0; i < deg->member_count; i++) {
        if (deg->members[i] == thread) {
            return i;
        }
    }
    return -1;
}

int deg_release_if_ready(dist_exec_group_t* deg) {
    if (!deg || deg->state != DEG_WAITING_RELEASE) {
        return 0;
    }

    uint32_t required_mask = (1U << deg->member_count) - 1;
    if ((deg->ready_bitmap & required_mask) == required_mask) {
        // All members are ready. Release!
        deg->state = DEG_ACTIVE;
        deg->release_epoch++;
        deg->ready_bitmap = 0;

        for (uint32_t i = 0; i < deg->member_count; i++) {
            struct kthread* member = deg->members[i];
            if (member && member->state == THREAD_STATE_DEG_PENDING) {
                member->state = THREAD_STATE_READY;
                // Enqueue to the actual core runqueue
                sched_enqueue(member, member->bound_core_id);
            }
        }
        return 1; // Released
    }
    return 0; // Not yet ready
}

int deg_mark_ready(struct kthread* thread) {
    if (!thread || !thread->sched_ctx || !thread->sched_ctx->deg) {
        return -1;
    }

    dist_exec_group_t* deg = thread->sched_ctx->deg;

    int idx = _deg_get_member_index(deg, thread);
    if (idx < 0) return -1;

    deg->ready_bitmap |= (1U << idx);

    // Check if we can release
    return deg_release_if_ready(deg);
}

void deg_abort_epoch(dist_exec_group_t* deg) {
    if (!deg) return;
    deg->state = DEG_ABORTED;
    deg->ready_bitmap = 0;

    for (uint32_t i = 0; i < deg->member_count; i++) {
        struct kthread* member = deg->members[i];
        if (member && member->state == THREAD_STATE_DEG_PENDING) {
            // Un-pending them or handle fallback
            // In a full implementation, we might schedule them as best-effort or degraded.
            // For MVP, if they were waiting, they drop to ready but isolated
            member->state = THREAD_STATE_READY;
            sched_enqueue(member, member->bound_core_id);
        }
    }

    deg->state = DEG_WAITING_RELEASE; // Reset for next try
}

void deg_block_member(struct kthread* thread, uint32_t reason) {
    (void)reason;
    if (!thread || !thread->sched_ctx || !thread->sched_ctx->deg) {
        return;
    }

    dist_exec_group_t* deg = thread->sched_ctx->deg;
    int idx = _deg_get_member_index(deg, thread);
    if (idx < 0) return;

    deg->running_bitmap &= ~(1U << idx);

    if (deg->mode == DEG_STRICT) {
        // A member blocked, so the strict co-scheduling epoch is broken
        deg_abort_epoch(deg);
    }
}
