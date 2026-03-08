#include "../../include/ipc_async.h"

// Note: Ensure the array declaration is reachable. For simplicity, we can declare
// the global array and size in a shared header or here as extern.
// To keep it simple, we define it in async_ipc.c and access it here.
extern ipc_async_request_t g_async_requests[];

#define MAX_ASYNC_REQUESTS 64U

void bharat_rt_deadline_timeout_hook(uint32_t endpoint_ref, uint32_t request_id, uint64_t current_ticks) __attribute__((weak));

void ipc_async_check_timeouts(uint64_t current_ticks) {
    for (uint32_t i = 0; i < MAX_ASYNC_REQUESTS; ++i) {
        ipc_async_request_t* req = &g_async_requests[i];
        if (req->in_use && req->state == IPC_ASYNC_STATE_PENDING) {
            if (req->deadline_ticks > 0 && current_ticks >= req->deadline_ticks) {
                req->state = IPC_ASYNC_STATE_TIMEOUT;
                req->in_use = 0U;

                if (bharat_rt_deadline_timeout_hook) {
                    bharat_rt_deadline_timeout_hook(req->endpoint_ref, req->id, current_ticks);
                }

                if (req->waiting_thread && req->waiting_thread->state == THREAD_STATE_BLOCKED) {
                    sched_wakeup_with_priority(req->waiting_thread, req->qos_priority);
                }
            }
        }
    }
}
