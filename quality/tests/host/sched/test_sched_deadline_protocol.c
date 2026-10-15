#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "sched/sched.h"
#include "sched_completion_protocol.h"
#include "time/ktime.h"

int main(void) {
    sched_rq_t rq;
    memset(&rq, 0, sizeof(rq));

    // Set up a pending command
    rq.outbound_cmds[5].state = SCHED_REMOTE_CMD_PENDING;
    rq.outbound_cmds[5].handle.generation = 42;
    rq.outbound_cmds[5].deadline_ns = 100;

    // Set up a completion cell for ACK
    rq.completions[5].state = COMPLETION_PUBLISHED;
    rq.completions[5].generation = 42;
    rq.completions[5].kind = SCHED_COMPLETION_ACK;
    rq.completions[5].result = 99;

    // 1. Drain completions
    sched_protocol_drain_completions(&rq);
    assert(rq.outbound_cmds[5].state == SCHED_REMOTE_CMD_ACKED);
    assert(rq.outbound_cmds[5].result == 99);
    assert(rq.completions[5].state == COMPLETION_EMPTY);

    // 2. Set up a pending command for NACK
    rq.outbound_cmds[6].state = SCHED_REMOTE_CMD_PENDING;
    rq.outbound_cmds[6].handle.generation = 43;
    rq.completions[6].state = COMPLETION_PUBLISHED;
    rq.completions[6].generation = 43;
    rq.completions[6].kind = SCHED_COMPLETION_NACK;
    sched_protocol_drain_completions(&rq);
    assert(rq.outbound_cmds[6].state == SCHED_REMOTE_CMD_FAILED);

    // 3. Stale generation rejection
    rq.outbound_cmds[7].state = SCHED_REMOTE_CMD_PENDING;
    rq.outbound_cmds[7].handle.generation = 44;
    rq.completions[7].state = COMPLETION_PUBLISHED;
    rq.completions[7].generation = 43; // stale
    sched_protocol_drain_completions(&rq);
    assert(rq.outbound_cmds[7].state == SCHED_REMOTE_CMD_PENDING);

    // 4. Timeout test
    rq.outbound_cmds[8].state = SCHED_REMOTE_CMD_PENDING;
    rq.outbound_cmds[8].deadline_ns = 100;
    sched_protocol_process_deadlines(&rq, 50);
    assert(rq.outbound_cmds[8].state == SCHED_REMOTE_CMD_PENDING);
    sched_protocol_process_deadlines(&rq, 100);
    assert(rq.outbound_cmds[8].state == SCHED_REMOTE_CMD_TIMEOUT);

    // 5. Infinite timeout
    rq.outbound_cmds[9].state = SCHED_REMOTE_CMD_PENDING;
    rq.outbound_cmds[9].deadline_ns = BH_KDEADLINE_INFINITE;
    sched_protocol_process_deadlines(&rq, 0xFFFFFFFFFFFFFFFFULL);
    assert(rq.outbound_cmds[9].state == SCHED_REMOTE_CMD_PENDING);

    // 6. Zero timeout
    rq.outbound_cmds[10].state = SCHED_REMOTE_CMD_PENDING;
    rq.outbound_cmds[10].deadline_ns = 0;
    sched_protocol_process_deadlines(&rq, 0);
    assert(rq.outbound_cmds[10].state == SCHED_REMOTE_CMD_TIMEOUT);

    printf("ALL TESTS PASSED\n");
    return 0;
}
