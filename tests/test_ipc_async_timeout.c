#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/ipc_async.h"
#include "../kernel/include/sched.h"

static uint64_t g_ticks = 0;
static int g_wakeup_count = 0;

uint64_t sched_get_ticks(void) {
    return g_ticks;
}

void sched_wakeup(kthread_t* thread) {
    if (thread) {
        thread->state = THREAD_STATE_READY;
    }
    g_wakeup_count++;
}

int main(void) {
    ipc_async_init();

    kthread_t thread = {0};
    thread.state = THREAD_STATE_BLOCKED;

    g_ticks = 10;
    ipc_async_request_t* req = ipc_async_request_create(&thread, 7U, 5U);
    assert(req != NULL);
    assert(req->deadline_ticks == 15U);

    ipc_async_check_timeouts(14U);
    assert(req->in_use == 1U);
    assert(req->state == IPC_ASYNC_STATE_PENDING);

    ipc_async_check_timeouts(15U);
    assert(req->in_use == 0U);
    assert(req->state == IPC_ASYNC_STATE_TIMEOUT);
    assert(g_wakeup_count == 1);
    assert(thread.state == THREAD_STATE_READY);

    printf("Async IPC timeout tests passed.\n");
    return 0;
}
