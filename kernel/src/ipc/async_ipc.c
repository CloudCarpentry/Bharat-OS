#include "../../include/ipc_async.h"
#include "../../include/sched.h"
#include <stddef.h>

#define MAX_ASYNC_REQUESTS 64U

ipc_async_request_t g_async_requests[MAX_ASYNC_REQUESTS];
static uint32_t g_next_async_id = 1U;
static uint32_t g_free_async_head = 0U;

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
    req->next_index = g_free_async_head;
    g_free_async_head = idx;
}

void ipc_async_init(void) {
    for (uint32_t i = 0; i < MAX_ASYNC_REQUESTS; i++) {
        g_async_requests[i].in_use = 0U;
        g_async_requests[i].next_index = (i + 1U < MAX_ASYNC_REQUESTS) ? (i + 1U) : UINT32_MAX;
    }
    g_free_async_head = 0U;
}

ipc_async_request_t* ipc_async_request_create_ex(kthread_t* thread,
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
    return req;
}

ipc_async_request_t* ipc_async_request_create(kthread_t* thread, uint32_t endpoint_ref, uint64_t timeout_ms) {
    return ipc_async_request_create_ex(thread, endpoint_ref, timeout_ms, 0U, 0U);
}

void ipc_async_request_complete(ipc_async_request_t* req) {
    if (req && req->in_use && req->state == IPC_ASYNC_STATE_PENDING) {
        kthread_t* waiting = req->waiting_thread;
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
        kthread_t* waiting = req->waiting_thread;
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
        kthread_t* waiting = req->waiting_thread;
        uint32_t qos = req->qos_priority;
        req->state = IPC_ASYNC_STATE_TIMEOUT;
        ipc_async_free_slot(req);
        if (waiting && waiting->state == THREAD_STATE_BLOCKED) {
            sched_wakeup_with_priority(waiting, qos);
        }
    }
}
