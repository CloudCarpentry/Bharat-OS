#include "../../include/ipc_async.h"
#include "../../include/sched.h"
#include <stddef.h>

#define MAX_ASYNC_REQUESTS 64U

ipc_async_request_t g_async_requests[MAX_ASYNC_REQUESTS];
static uint32_t g_next_async_id = 1U;

void ipc_async_init(void) {
    for (uint32_t i = 0; i < MAX_ASYNC_REQUESTS; i++) {
        g_async_requests[i].in_use = 0U;
    }
}

ipc_async_request_t* ipc_async_request_create_ex(kthread_t* thread,
                                                  uint32_t endpoint_ref,
                                                  uint64_t timeout_ms,
                                                  uint32_t qos_priority,
                                                  uint8_t deterministic) {
    if (!thread) return NULL;

    for (uint32_t i = 0; i < MAX_ASYNC_REQUESTS; i++) {
        if (g_async_requests[i].in_use == 0U) {
            g_async_requests[i].in_use = 1U;
            g_async_requests[i].id = g_next_async_id++;
            g_async_requests[i].state = IPC_ASYNC_STATE_PENDING;
            g_async_requests[i].waiting_thread = thread;
            if (timeout_ms == 0U) {
                g_async_requests[i].deadline_ticks = 0U;
            } else {
                uint64_t now = sched_get_ticks();
                g_async_requests[i].deadline_ticks = now + timeout_ms;
            }
            g_async_requests[i].endpoint_ref = endpoint_ref;
            g_async_requests[i].qos_priority = qos_priority;
            g_async_requests[i].deterministic = deterministic ? 1U : 0U;
            return &g_async_requests[i];
        }
    }
    return NULL;
}

ipc_async_request_t* ipc_async_request_create(kthread_t* thread, uint32_t endpoint_ref, uint64_t timeout_ms) {
    return ipc_async_request_create_ex(thread, endpoint_ref, timeout_ms, 0U, 0U);
}

void ipc_async_request_complete(ipc_async_request_t* req) {
    if (req && req->in_use && req->state == IPC_ASYNC_STATE_PENDING) {
        req->state = IPC_ASYNC_STATE_COMPLETED;
        req->in_use = 0U;
        // Wake up thread if blocked
        if (req->waiting_thread && req->waiting_thread->state == THREAD_STATE_BLOCKED) {
            sched_wakeup_with_priority(req->waiting_thread, req->qos_priority);
        }
    }
}

void ipc_async_request_cancel(ipc_async_request_t* req) {
    if (req && req->in_use && req->state == IPC_ASYNC_STATE_PENDING) {
        req->state = IPC_ASYNC_STATE_CANCELLED;
        req->in_use = 0U;
        if (req->waiting_thread && req->waiting_thread->state == THREAD_STATE_BLOCKED) {
            sched_wakeup_with_priority(req->waiting_thread, req->qos_priority);
        }
    }
}
