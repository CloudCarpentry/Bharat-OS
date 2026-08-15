/* SPDX-License-Identifier: MIT */
#include "sched/sched_diag.h"

#include "hal/hal.h"

typedef struct sched_diag_core_state {
  uint32_t runtime_level;
  uint32_t last_event;
  uint32_t event_count[BH_SCHED_DIAG_EVENT_COUNT];
} sched_diag_core_state_t;

/* Owner-local mutable state: slot N is written only by CPU N. */
static sched_diag_core_state_t g_sched_diag[MAX_SUPPORTED_CORES];

void bh_sched_diag_init(void) {
  for (uint32_t core = 0; core < MAX_SUPPORTED_CORES; ++core) {
    __atomic_store_n(&g_sched_diag[core].runtime_level, BH_SCHED_DIAG_OFF,
                     __ATOMIC_RELAXED);
    __atomic_store_n(&g_sched_diag[core].last_event,
                     BH_SCHED_DIAG_EVENT_COUNT, __ATOMIC_RELAXED);
    for (uint32_t event = 0; event < BH_SCHED_DIAG_EVENT_COUNT; ++event) {
      __atomic_store_n(&g_sched_diag[core].event_count[event], 0U,
                       __ATOMIC_RELAXED);
    }
  }
}

void bh_sched_diag_set_level(bh_sched_diag_level_t level) {
  uint32_t core = hal_cpu_get_id();
  if (core >= MAX_SUPPORTED_CORES) {
    return;
  }
  if ((uint32_t)level > (uint32_t)BHARAT_SCHED_DIAG_COMPILE_LEVEL) {
    level = (bh_sched_diag_level_t)BHARAT_SCHED_DIAG_COMPILE_LEVEL;
  }
  __atomic_store_n(&g_sched_diag[core].runtime_level, (uint32_t)level,
                   __ATOMIC_RELAXED);
}

void bh_sched_diag_record(uint32_t core_id, bh_sched_diag_level_t level,
                          bh_sched_diag_event_t event) {
  if (core_id >= MAX_SUPPORTED_CORES || event >= BH_SCHED_DIAG_EVENT_COUNT ||
      level == BH_SCHED_DIAG_OFF) {
    return;
  }
  uint32_t runtime_level =
      __atomic_load_n(&g_sched_diag[core_id].runtime_level, __ATOMIC_RELAXED);
  if (runtime_level < (uint32_t)level) {
    return;
  }
  __atomic_add_fetch(&g_sched_diag[core_id].event_count[event], 1U,
                     __ATOMIC_RELAXED);
  if (level == BH_SCHED_DIAG_TRACE) {
    __atomic_store_n(&g_sched_diag[core_id].last_event, (uint32_t)event,
                     __ATOMIC_RELAXED);
  }
}

void bh_sched_diag_snapshot(uint32_t core_id,
                            bh_sched_diag_snapshot_t *snapshot) {
  if (!snapshot || core_id >= MAX_SUPPORTED_CORES) {
    return;
  }
  snapshot->runtime_level =
      __atomic_load_n(&g_sched_diag[core_id].runtime_level, __ATOMIC_RELAXED);
  snapshot->last_event =
      __atomic_load_n(&g_sched_diag[core_id].last_event, __ATOMIC_RELAXED);
  for (uint32_t event = 0; event < BH_SCHED_DIAG_EVENT_COUNT; ++event) {
    snapshot->event_count[event] = __atomic_load_n(
        &g_sched_diag[core_id].event_count[event], __ATOMIC_RELAXED);
  }
}
