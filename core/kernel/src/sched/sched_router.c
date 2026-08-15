#include "sched/sched.h"
#include "sched_internal.h"

void sched_set_policy(sched_policy_t policy) {
  if (policy <= SCHED_POLICY_RMS) {
    uint32_t core = sched_current_core_or_panic();
    g_cpu_locals[core].runqueue.policy = policy;
  }
}

sched_policy_t sched_get_policy(void) {
  return sched_policy_for_core(hal_cpu_get_id());
}
