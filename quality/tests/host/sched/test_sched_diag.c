/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>

#include "sched/sched_diag.h"

static uint32_t current_core;

uint32_t hal_cpu_get_id(void) { return current_core; }

int main(void) {
  bh_sched_diag_snapshot_t snapshot;

  bh_sched_diag_init();
  current_core = 1U;
  bh_sched_diag_set_level(BH_SCHED_DIAG_COUNTERS);
  BH_DIAG_COUNTER(1U, BH_SCHED_DIAG_PICK_NON_IDLE);
  BH_TRACE_SCHED(1U, BH_SCHED_DIAG_SWITCH_BEGIN);
  bh_sched_diag_snapshot(1U, &snapshot);
  assert(snapshot.runtime_level == BH_SCHED_DIAG_COUNTERS);
  assert(snapshot.event_count[BH_SCHED_DIAG_PICK_NON_IDLE] == 1U);
  assert(snapshot.event_count[BH_SCHED_DIAG_SWITCH_BEGIN] == 0U);

  bh_sched_diag_set_level(BH_SCHED_DIAG_TRACE);
  BH_TRACE_SCHED(1U, BH_SCHED_DIAG_CONTEXT_SWITCH);
  bh_sched_diag_snapshot(1U, &snapshot);
  assert(snapshot.last_event == BH_SCHED_DIAG_CONTEXT_SWITCH);
  assert(snapshot.event_count[BH_SCHED_DIAG_CONTEXT_SWITCH] == 1U);

  current_core = 0U;
  bh_sched_diag_set_level(BH_SCHED_DIAG_TRACE);
  bh_sched_diag_snapshot(1U, &snapshot);
  assert(snapshot.runtime_level == BH_SCHED_DIAG_TRACE);
  return 0;
}
