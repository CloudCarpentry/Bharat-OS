#include "../../include/ipc_async.h"

void bharat_rt_deadline_timeout_hook(uint32_t endpoint_ref, uint32_t request_id, uint64_t current_ticks) __attribute__((weak));

void ipc_async_check_timeouts(uint64_t current_ticks) {
    ipc_async_request_t* req = ipc_async_timeout_peek();
    while (req && req->in_use && req->state == IPC_ASYNC_STATE_PENDING &&
           req->deadline_ticks > 0U && current_ticks >= req->deadline_ticks) {
        uint32_t endpoint_ref = req->endpoint_ref;
        uint32_t request_id = req->id;

        if (bharat_rt_deadline_timeout_hook) {
            bharat_rt_deadline_timeout_hook(endpoint_ref, request_id, current_ticks);
        }
        ipc_async_request_timeout(req, current_ticks);
        req = ipc_async_timeout_peek();
    }
}
