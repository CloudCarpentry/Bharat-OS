#include "sched/sched.h"
#include "sched/sched_invariants.h"
#include "sched/sched_diag.h"
#include "sched_internal.h"
#include "panic.h"

sched_policy_t sched_policy_for_core(uint32_t core_id) {
  return g_cpu_locals[sched_clamp_core(core_id)].runqueue.policy;
}

bh_thread_t *sched_edf_pick_next(sched_rq_t *rq) {
    struct rb_node *left = rb_first(&rq->edf_runqueue);
    if (!left) {
        return NULL;
    }
    sched_entity_t *entity = (sched_entity_t *)(void *)((char *)left - offsetof(sched_entity_t, edf_node));
    return sched_find_thread_by_id(entity->tid);
}

bh_thread_t *sched_cfs_pick_next(sched_rq_t *rq) {
    struct rb_node *left = rb_first(&rq->cfs_runqueue);
    if (!left) {
        return NULL;
    }
    sched_entity_t *entity = (sched_entity_t *)(void *)((char *)left - offsetof(sched_entity_t, cfs_node));
    return sched_find_thread_by_id(entity->tid);
}

bh_thread_t *sched_pick_next_ready(uint32_t core_id) {
  core_id = sched_clamp_core(core_id);
  sched_rq_t *rq = &g_cpu_locals[core_id].runqueue;

  bh_thread_t *next = NULL;

  if (rq->policy == SCHED_POLICY_CLOUD_FAIR) {
      next = sched_cfs_pick_next(rq);
      if (next) {
          sched_invariant_on_dequeue(next);
          sched_cfs_dequeue(rq, next);
      }
  } else if (rq->policy == SCHED_POLICY_EDF) {
      next = sched_edf_pick_next(rq);
      if (next) {
          sched_invariant_on_dequeue(next);
          sched_edf_dequeue(rq, next);
      }
  } else {
      int pick_highest = (rq->policy == SCHED_POLICY_ROUND_ROBIN) ? 0 : 1;
      int prio = sched_pick_priority_from_bitmap(rq, pick_highest);
      if (prio >= 0) {
          list_head_t *head = &rq->ready_queue[prio];
          list_head_t *node = head->prev;
          sched_entity_t *entity = (sched_entity_t *)(void *)((char *)node - offsetof(sched_entity_t, run_node));
          bh_thread_t *thread_picked = sched_find_thread_by_id(entity->tid);
          if (thread_picked) {
              sched_invariant_on_dequeue(thread_picked);
              list_del(node);
              list_init(node);
              sched_ready_bitmap_clear_if_empty(rq, (uint32_t)prio);
              next = thread_picked;
              entity->is_on_runqueue = 0U;
              if (rq->runnable_count > 0) {
                  rq->runnable_count--;
              }
          }
      }
  }

  if (!next) {
      return rq->idle_thread;
  }

  BH_DIAG_COUNTER(core_id, BH_SCHED_DIAG_PICK_NON_IDLE);
  return sched_validate_picked_candidate(next, rq->idle_thread, core_id);
}

bh_thread_t *sched_pick_next_ready_l0(uint32_t core_id) {
  return sched_pick_next_ready(core_id);
}

bh_thread_t *sched_pick_next_ready_l1(uint32_t core_id) {
  return sched_pick_next_ready(core_id);
}
