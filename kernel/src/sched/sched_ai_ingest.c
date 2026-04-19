#include "sched/sched.h"
#include "sched_internal.h"

int sched_suggestion_dequeue(ai_suggestion_t *out) {
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;
  suggestion_queue_t *sq = (suggestion_queue_t *)rq->pending_suggestions;

  if (!out || sq->head == sq->tail) {
    return -1;
  }
  *out = sq->queue[sq->tail];
  sq->tail = (sq->tail + 1U) % SCHED_MAX_PENDING_SUGGESTIONS;
  return 0;
}

void sched_process_pending_ai_suggestions(void) {
  ai_suggestion_t suggestion = {0};
  while (sched_suggestion_dequeue(&suggestion) == 0) {
    (void)sched_ai_apply_suggestion(&suggestion);
  }
}

int sched_enqueue_ai_suggestion(const ai_suggestion_t *suggestion) {
  if (!suggestion) {
    return -1;
  }
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;
  suggestion_queue_t *sq = (suggestion_queue_t *)rq->pending_suggestions;

  uint32_t next = (sq->head + 1U) % SCHED_MAX_PENDING_SUGGESTIONS;
  if (next == sq->tail) {
    return -2;
  }
  sq->queue[sq->head] = *suggestion;
  sq->head = next;
  return 0;
}

void sched_notify_ipc_ready(uint32_t core_id, uint32_t msg_type) {
  (void)core_id;
  (void)msg_type;
}

