/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_SCHED_DIAG_H
#define BHARAT_SCHED_DIAG_H

#include <stdint.h>

#include "bharat_config.h"

#ifndef BHARAT_SCHED_DIAG_COMPILE_LEVEL
#define BHARAT_SCHED_DIAG_COMPILE_LEVEL 0
#endif

#if BHARAT_SCHED_DIAG_COMPILE_LEVEL > 2
#error "BHARAT_SCHED_DIAG_COMPILE_LEVEL must be in the range 0..2"
#endif

typedef enum bh_sched_diag_level {
  BH_SCHED_DIAG_OFF = 0,
  BH_SCHED_DIAG_COUNTERS = 1,
  BH_SCHED_DIAG_TRACE = 2,
} bh_sched_diag_level_t;

typedef enum bh_sched_diag_event {
  BH_SCHED_DIAG_PICK_NON_IDLE = 0,
  BH_SCHED_DIAG_SWITCH_BEGIN,
  BH_SCHED_DIAG_EXT_STATE_SAVE,
  BH_SCHED_DIAG_ASPACE_SWITCH,
  BH_SCHED_DIAG_URPC_DRAIN,
  BH_SCHED_DIAG_CONTEXT_SWITCH,
  BH_SCHED_DIAG_EVENT_COUNT,
} bh_sched_diag_event_t;

typedef struct bh_sched_diag_snapshot {
  uint32_t runtime_level;
  uint32_t last_event;
  uint64_t event_count[BH_SCHED_DIAG_EVENT_COUNT];
} bh_sched_diag_snapshot_t;

/*
 * Diagnostic state is partitioned by core. Only the owner core records events
 * or changes its runtime level; snapshots use atomic loads for remote readers.
 */
void bh_sched_diag_init(void);
void bh_sched_diag_set_level(bh_sched_diag_level_t level);
void bh_sched_diag_record(uint32_t core_id, bh_sched_diag_level_t level,
                          bh_sched_diag_event_t event);
void bh_sched_diag_snapshot(uint32_t core_id,
                            bh_sched_diag_snapshot_t *snapshot);

#if BHARAT_SCHED_DIAG_COMPILE_LEVEL >= 1
#define BH_DIAG_COUNTER(core_id, event)                                      \
  bh_sched_diag_record((core_id), BH_SCHED_DIAG_COUNTERS, (event))
#else
#define BH_DIAG_COUNTER(core_id, event) ((void)0)
#endif

#if BHARAT_SCHED_DIAG_COMPILE_LEVEL >= 2
#define BH_TRACE_SCHED(core_id, event)                                       \
  bh_sched_diag_record((core_id), BH_SCHED_DIAG_TRACE, (event))
#else
#define BH_TRACE_SCHED(core_id, event) ((void)0)
#endif

#endif /* BHARAT_SCHED_DIAG_H */
