#include "../../include/ipc_async.h"
#include "sched/sched.h"
#include <stddef.h>

#define MAX_ASYNC_REQUESTS 64U

ipc_async_request_t g_async_requests[MAX_ASYNC_REQUESTS];
static uint32_t g_next_async_id = 1U;
static uint32_t g_free_async_head = 0U;
static uint32_t g_timeout_heap[MAX_ASYNC_REQUESTS];
static uint32_t g_timeout_heap_size = 0U;

static inline uint64_t ipc_async_deadline_by_heap_pos(uint32_t heap_pos) {
    uint32_t req_idx = g_timeout_heap[heap_pos];
    return g_async_requests[req_idx].deadline_ticks;
}

static void ipc_async_heap_swap(uint32_t a, uint32_t b) {
    uint32_t idx_a = g_timeout_heap[a];
    uint32_t idx_b = g_timeout_heap[b];
    g_timeout_heap[a] = idx_b;
    g_timeout_heap[b] = idx_a;
    g_async_requests[idx_a].timeout_heap_pos = b;
    g_async_requests[idx_b].timeout_heap_pos = a;
}

static void ipc_async_heap_sift_up(uint32_t pos) {
    while (pos > 0U) {
        uint32_t parent = (pos - 1U) / 2U;
        if (ipc_async_deadline_by_heap_pos(parent) <= ipc_async_deadline_by_heap_pos(pos)) {
            break;
        }
        ipc_async_heap_swap(parent, pos);
        pos = parent;
    }
}

static void ipc_async_heap_sift_down(uint32_t pos) {
    for (;;) {
        uint32_t left = (2U * pos) + 1U;
        uint32_t right = left + 1U;
        uint32_t smallest = pos;
        if (left < g_timeout_heap_size &&
            ipc_async_deadline_by_heap_pos(left) < ipc_async_deadline_by_heap_pos(smallest)) {
            smallest = left;
        }
        if (right < g_timeout_heap_size &&
            ipc_async_deadline_by_heap_pos(right) < ipc_async_deadline_by_heap_pos(smallest)) {
            smallest = right;
        }
        if (smallest == pos) {
            break;
        }
        ipc_async_heap_swap(pos, smallest);
        pos = smallest;
    }
}

static void ipc_async_timeout_track(ipc_async_request_t* req) {
    if (!req || req->deadline_ticks == 0U) {
        return;
    }
    uint32_t req_idx = (uint32_t)(req - &g_async_requests[0]);
    req->timeout_heap_pos = g_timeout_heap_size;
    g_timeout_heap[g_timeout_heap_size++] = req_idx;
    ipc_async_heap_sift_up(req->timeout_heap_pos);
}

static void ipc_async_timeout_untrack(ipc_async_request_t* req) {
    if (!req || req->timeout_heap_pos >= g_timeout_heap_size) {
        return;
    }

    uint32_t pos = req->timeout_heap_pos;
    uint32_t last_pos = g_timeout_heap_size - 1U;
    uint32_t last_idx = g_timeout_heap[last_pos];
    req->timeout_heap_pos = UINT32_MAX;

    if (pos != last_pos) {
        g_timeout_heap[pos] = last_idx;
        g_async_requests[last_idx].timeout_heap_pos = pos;
    }
    g_timeout_heap_size--;

    if (pos < g_timeout_heap_size) {
        if (pos > 0U && ipc_async_deadline_by_heap_pos(pos) <
                             ipc_async_deadline_by_heap_pos((pos - 1U) / 2U)) {
            ipc_async_heap_sift_up(pos);
        } else {
            ipc_async_heap_sift_down(pos);
        }
    }
}

static ipc_async_request_t* ipc_async_alloc_slot(void) {
    if (g_free_async_head >= MAX_ASYNC_REQUESTS) {
        return NULL;
    }
    uint32_t idx = g_free_async_head;
    ipc_async_request_t* req = &g_async_requests[idx];
    g_free_async_head = req->next_index;
    req->next_index = UINT32_MAX;
    return req;
}

static void ipc_async_free_slot(ipc_async_request_t* req) {
    if (!req) {
        return;
    }
    uint32_t idx = (uint32_t)(req - &g_async_requests[0]);
    req->in_use = 0U;
    req->waiting_thread = NULL;
    req->deadline_ticks = 0U;
    ipc_async_timeout_untrack(req);
    req->next_index = g_free_async_head;
    g_free_async_head = idx;
}

void ipc_async_init(void) {
    for (uint32_t i = 0; i < MAX_ASYNC_REQUESTS; i++) {
        g_async_requests[i].in_use = 0U;
        g_async_requests[i].next_index = (i + 1U < MAX_ASYNC_REQUESTS) ? (i + 1U) : UINT32_MAX;
        g_async_requests[i].timeout_heap_pos = UINT32_MAX;
    }
    g_free_async_head = 0U;
    g_timeout_heap_size = 0U;
    g_next_async_id = 1U;
}

ipc_async_request_t* ipc_async_request_create_ex(bh_thread_t* thread,
                                                  uint32_t endpoint_ref,
                                                  uint64_t timeout_ms,
                                                  uint32_t qos_priority,
                                                  uint8_t deterministic) {
    if (!thread) return NULL;

    ipc_async_request_t* req = ipc_async_alloc_slot();
    if (!req) {
        return NULL;
    }

    req->in_use = 1U;
    req->id = g_next_async_id++;
    req->state = IPC_ASYNC_STATE_PENDING;
    req->waiting_thread = thread;
    if (timeout_ms == 0U) {
        req->deadline_ticks = 0U;
    } else {
        uint64_t now = sched_get_ticks();
        req->deadline_ticks = now + timeout_ms;
    }
    req->endpoint_ref = endpoint_ref;
    req->qos_priority = qos_priority;
    req->deterministic = deterministic ? 1U : 0U;
    req->timeout_heap_pos = UINT32_MAX;

    ipc_async_timeout_track(req);
    return req;
}

ipc_async_request_t* ipc_async_request_create(bh_thread_t* thread, uint32_t endpoint_ref, uint64_t timeout_ms) {
    return ipc_async_request_create_ex(thread, endpoint_ref, timeout_ms, 0U, 0U);
}

void ipc_async_request_complete(ipc_async_request_t* req) {
    if (req && req->in_use && req->state == IPC_ASYNC_STATE_PENDING) {
        bh_thread_t* waiting = req->waiting_thread;
        uint32_t qos = req->qos_priority;
        req->state = IPC_ASYNC_STATE_COMPLETED;
        ipc_async_free_slot(req);
        // Wake up thread if blocked
        if (waiting && waiting->state == THREAD_STATE_BLOCKED) {
            sched_wakeup_with_priority(waiting, qos);
        }
    }
}

void ipc_async_request_cancel(ipc_async_request_t* req) {
    if (req && req->in_use && req->state == IPC_ASYNC_STATE_PENDING) {
        bh_thread_t* waiting = req->waiting_thread;
        uint32_t qos = req->qos_priority;
        req->state = IPC_ASYNC_STATE_CANCELLED;
        ipc_async_free_slot(req);
        if (waiting && waiting->state == THREAD_STATE_BLOCKED) {
            sched_wakeup_with_priority(waiting, qos);
        }
    }
}

void ipc_async_request_timeout(ipc_async_request_t* req, uint64_t current_ticks) {
    (void)current_ticks;
    if (req && req->in_use && req->state == IPC_ASYNC_STATE_PENDING) {
        bh_thread_t* waiting = req->waiting_thread;
        uint32_t qos = req->qos_priority;
        req->state = IPC_ASYNC_STATE_TIMEOUT;
        ipc_async_free_slot(req);
        if (waiting && waiting->state == THREAD_STATE_BLOCKED) {
            sched_wakeup_with_priority(waiting, qos);
        }
    }
}

ipc_async_request_t* ipc_async_timeout_peek(void) {
    if (g_timeout_heap_size == 0U) {
        return NULL;
    }
    return &g_async_requests[g_timeout_heap[0]];
}
