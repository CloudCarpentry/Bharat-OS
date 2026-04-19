#include "sched/sched.h"
#include "sched_internal.h"

int sched_ai_apply_suggestion(const ai_suggestion_t *suggestion) {
  if (!suggestion) {
    return -1;
  }
  kthread_t *thread = sched_find_thread_by_id((uint64_t)suggestion->target_id);
  if (!thread) {
      return -1;
  }
  switch (suggestion->action) {
  case AI_ACTION_ADJUST_PRIORITY:
    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
        if (suggestion->value == 0) return -1;
        thread->weight = suggestion->value;
        return 0;
    }
    return sched_adjust_priority(thread, suggestion->value);
  case AI_ACTION_MIGRATE_TASK:
    return sched_migrate_task(thread, suggestion->value);
  case AI_ACTION_THROTTLE_CORE:
    return sched_throttle_core(suggestion->value);
  case AI_ACTION_KILL_TASK:
    if (sched_mark_thread_terminated(thread) != 0) {
      return -1;
    }
    if (thread == sched_current_thread()) {
      uint32_t core = sched_clamp_core(hal_cpu_get_id());
      g_cpu_locals[core].runqueue.current_thread = NULL;
      sched_reschedule();
    }
    return 0;
  case AI_ACTION_NONE:
  default:
    return -1;
  }
}

