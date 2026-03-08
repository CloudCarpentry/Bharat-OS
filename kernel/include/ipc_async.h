#ifndef BHARAT_ASYNC_IPC_H
#define BHARAT_ASYNC_IPC_H

#include <stdint.h>
#include "sched.h"

typedef enum {
    IPC_ASYNC_STATE_PENDING,
    IPC_ASYNC_STATE_COMPLETED,
    IPC_ASYNC_STATE_CANCELLED,
    IPC_ASYNC_STATE_TIMEOUT
} ipc_async_state_t;

typedef struct {
    uint32_t id;
    ipc_async_state_t state;
    kthread_t* waiting_thread;
    uint64_t deadline_ticks;
    uint32_t endpoint_ref;
    uint8_t in_use;
} ipc_async_request_t;

/*
 * Initialize the Async IPC subsystem.
 */
void ipc_async_init(void);

/*
 * Create an asynchronous IPC request object.
 */
ipc_async_request_t* ipc_async_request_create(kthread_t* thread, uint32_t endpoint_ref, uint64_t timeout_ms);

/*
 * Complete an IPC request.
 */
void ipc_async_request_complete(ipc_async_request_t* req);

/*
 * Cancel an IPC request (due to user request or subsystem teardown).
 */
void ipc_async_request_cancel(ipc_async_request_t* req);

/*
 * Called by the scheduler to check for expired deadlines and handle timeouts.
 */
void ipc_async_check_timeouts(uint64_t current_ticks);

#endif // BHARAT_ASYNC_IPC_H
