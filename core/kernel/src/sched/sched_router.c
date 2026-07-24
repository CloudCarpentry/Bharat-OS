#include "sched/sched.h"
#include "sched_internal.h"

void sched_set_policy(sched_policy_t policy) {
  if (policy <= SCHED_POLICY_RMS) {
    g_policy = policy;
  }
}

