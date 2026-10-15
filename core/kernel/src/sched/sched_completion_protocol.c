#include "sched_completion_protocol.h"

void sched_protocol_drain_completions(sched_rq_t *rq) {
  for (uint32_t i = 0; i < SCHED_REMOTE_CMD_CAPACITY; ++i) {
    sched_remote_cmd_t *cmd = &rq->outbound_cmds[i];
    uint32_t cmd_state = __atomic_load_n(&cmd->state, __ATOMIC_ACQUIRE);

    if (cmd_state == SCHED_REMOTE_CMD_PENDING) {
      sched_completion_cell_t *cell = &rq->completions[i];
      uint32_t cell_state = __atomic_load_n(&cell->state, __ATOMIC_ACQUIRE);

      if (cell_state == COMPLETION_PUBLISHED) {
        if (cell->generation == cmd->handle.generation) {
          uint32_t next_state;
          if (cell->kind == SCHED_COMPLETION_ACK) {
              next_state = SCHED_REMOTE_CMD_ACKED;
          } else {
              next_state = SCHED_REMOTE_CMD_FAILED;
          }
          cmd->result = cell->result;

          __atomic_store_n(&cell->state, COMPLETION_EMPTY, __ATOMIC_RELEASE);
          __atomic_store_n(&cmd->state, next_state, __ATOMIC_RELEASE);
        }
      }
    }
  }
}

void sched_protocol_process_deadlines(sched_rq_t *rq, bh_ktime_t current_time) {
  for (uint32_t i = 0; i < SCHED_REMOTE_CMD_CAPACITY; ++i) {
    sched_remote_cmd_t *cmd = &rq->outbound_cmds[i];
    uint32_t state = __atomic_load_n(&cmd->state, __ATOMIC_ACQUIRE);

    if (state == SCHED_REMOTE_CMD_PENDING) {
      if (cmd->deadline_ns != BH_KDEADLINE_INFINITE && current_time >= cmd->deadline_ns) {
        __atomic_store_n(&cmd->state, SCHED_REMOTE_CMD_TIMEOUT, __ATOMIC_RELEASE);
      }
    }
  }
}
