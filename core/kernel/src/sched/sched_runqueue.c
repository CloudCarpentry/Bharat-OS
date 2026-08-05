#include "sched/sched.h"
#include "sched/sched_invariants.h"
#include "sched_internal.h"
#include "panic.h"

int sched_enqueue(bh_thread_t *thread, uint32_t core_id) {
  if (!thread || thread->priority >= MAX_PRIORITY_LEVELS) {
    return -1;
  }

  core_id = sched_clamp_core(core_id);
  if (!sched_is_core_admissible(thread, core_id)) {
    return -1; // SCHED_REJECT
  }
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  bool is_local = (core_id == current_core);

  if (!is_local) {
      if (sched_read_isolated_snapshot(core_id)) {
          return K_ERR_ISOLATED;
      }

      sched_invariant_check_remote_enqueue_path(thread);

      sched_remote_cmd_t *cmd = sched_allocate_outbound_cmd();
      if (!cmd) {
          return K_ERR_NO_RESOURCES;
      }

      cmd->type = SCHED_REMOTE_ENQUEUE;
      cmd->source_cpu = current_core;
      cmd->target_cpu = core_id;
      cmd->thread_id = thread->thread_id;
      cmd->expected_thread_generation = thread->sched_generation;
      cmd->priority = thread->priority;
      cmd->state = SCHED_REMOTE_CMD_PENDING;

      kstatus_t status = sched_remote_submit(core_id, cmd);
      if (status != K_OK) {
          sched_remote_cmd_release(cmd);
          return status;
      }
      return 0;
  }

  sched_rq_t *rq = sched_local_rq();

  hal_cpu_disable_interrupts();

  sched_entity_t *entity = sched_find_entity_by_thread(thread);
  if (!entity) {
    entity = sched_allocate_entity(current_core);
    if (!entity) {
      hal_cpu_enable_interrupts();
      return -1;
    }
    entity->tid = thread->thread_id;
    thread->owner_cpu = current_core;
    thread->owner_locator.cpu = (uint16_t)current_core;
    ptrdiff_t diff = (sched_entity_slot_t *)((char *)entity - offsetof(sched_entity_slot_t, entity)) - rq->entities;
    thread->owner_locator.slot = (uint16_t)diff;
    thread->owner_locator.entity_generation = entity->entity_generation;
    thread->owner_locator.migration_epoch = thread->migration_epoch;

    entity->priority = thread->priority;
    entity->base_priority = thread->base_priority;
    entity->affinity_mask = thread->affinity_mask;
    entity->vruntime = thread->vruntime;
    entity->absolute_deadline = thread->absolute_deadline_ms;
    entity->rt_attr = thread->rt_attr;
    if (thread->cpu_context && thread->cpu_context != &entity->context) {
      entity->context = *(cpu_context_t *)thread->cpu_context;
    }
    thread->cpu_context = &entity->context;
  }

  if (entity->is_on_runqueue != 0U) {
    sched_invariant_on_dequeue(thread);
    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
      sched_cfs_dequeue(rq, thread);
    } else if (g_policy == SCHED_POLICY_EDF) {
      sched_edf_dequeue(rq, thread);
    } else {
      list_del(&entity->run_node);
      list_init(&entity->run_node);
      sched_ready_bitmap_clear_if_empty(rq, entity->priority);
    }
    entity->is_on_runqueue = 0U;
    if (rq->runnable_count > 0U) {
      rq->runnable_count--;
    }
  }

  thread->bound_core_id = core_id;
  thread->state = THREAD_STATE_READY;
  entity->state = THREAD_STATE_READY;
  entity->runnable = true;

  sched_invariant_on_enqueue(thread, core_id);

  entity->absolute_deadline = thread->absolute_deadline_ms;
  entity->rt_attr = thread->rt_attr;

  if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
    sched_cfs_enqueue(rq, thread);
  } else if (g_policy == SCHED_POLICY_EDF) {
    if (thread->rt_attr.period_ms > 0 && thread->rt_attr.deadline_ms > 0) {
        if (thread->absolute_deadline_ms == 0) {
            thread->absolute_deadline_ms = rq->total_ticks + thread->rt_attr.deadline_ms;
            entity->absolute_deadline = thread->absolute_deadline_ms;
        }
    }
    sched_edf_enqueue(rq, thread);
  } else {
    list_add(&entity->run_node, &rq->ready_queue[entity->priority]);
    sched_ready_bitmap_set(rq, entity->priority);
  }

  entity->is_on_runqueue = 1U;
  rq->runnable_count++;

  sched_validate_rq(rq);

  hal_cpu_enable_interrupts();
  return 0;
}

uint32_t sched_run_queue_depth(uint32_t core_id) {
  sched_load_snapshot_t snap;
  if (sched_read_load_snapshot(core_id, &snap) == K_OK) {
    return snap.runnable_count;
  }
  return 0;
}

void sched_ready_bitmap_set(sched_rq_t *rq, uint32_t prio) {
  if (!rq || prio >= MAX_PRIORITY_LEVELS) {
    return;
  }
  rq->ready_bitmap |= (1U << prio);
}

void sched_ready_bitmap_clear_if_empty(sched_rq_t *rq, uint32_t prio) {
  if (!rq || prio >= MAX_PRIORITY_LEVELS) {
    return;
  }
  if (list_empty(&rq->ready_queue[prio])) {
    rq->ready_bitmap &= ~(1U << prio);
  }
}

int sched_pick_priority_from_bitmap(const sched_rq_t *rq, int highest) {
  if (!rq || rq->ready_bitmap == 0U) {
    return -1;
  }
  if (highest != 0) {
    return 31 - __builtin_clz(rq->ready_bitmap);
  }
  return __builtin_ctz(rq->ready_bitmap);
}

static void sched_dequeue_task_l0(bh_thread_t *thread, uint32_t core_id) {
  if (!thread) {
    return;
  }
  (void)core_id;
  sched_rq_t *rq = sched_local_rq();
  sched_entity_t *entity = sched_find_entity_by_thread(thread);
  if (entity && entity->is_on_runqueue != 0U) {
    sched_invariant_on_dequeue(thread);
    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
      sched_cfs_dequeue(rq, thread);
    } else {
      list_del(&entity->run_node);
      list_init(&entity->run_node);
      sched_ready_bitmap_clear_if_empty(rq, entity->priority);
    }
    entity->is_on_runqueue = 0U;
    if (rq->runnable_count > 0U) {
      rq->runnable_count--;
    }
  }
}

void sched_enqueue_task_l0(bh_thread_t *thread, uint32_t core_id) {
  (void)sched_enqueue(thread, core_id);
}

void sched_enqueue_task_l1(bh_thread_t *thread, uint32_t core_id) {
  (void)sched_enqueue(thread, core_id);
}

void sched_dequeue_task_l1(bh_thread_t *thread, uint32_t core_id) {
  sched_dequeue_task_l0(thread, core_id);
}

void sched_validate_rq(sched_rq_t *rq) {
    // Debug only
#ifndef NDEBUG
    if (!rq) return;

    // Valid count
    if (rq->runnable_count > SCHED_MAX_THREADS) {
        kernel_panic("Runqueue count invalid/underflow");
    }

    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
        // Validate min_vruntime is sensible
        struct rb_node *first = rb_first(&rq->cfs_runqueue);
        if (first) {
            bh_thread_t *next = (bh_thread_t *)(void *)((char *)first - offsetof(bh_thread_t, cfs_node));
            if (next->vruntime < rq->min_vruntime && rq->min_vruntime - next->vruntime > 1000) {
                // Minor drift is okay due to rounding, but major divergence is a bug
                kernel_panic("Runqueue min_vruntime divergence");
            }
        }
    } else {
        // Validate priority bitmaps vs lists
        for (uint32_t i = 0; i < MAX_PRIORITY_LEVELS; i++) {
            int is_empty = list_empty(&rq->ready_queue[i]);
            int bit_set = (rq->ready_bitmap & (1U << i)) != 0;
            if (is_empty && bit_set) {
                 kernel_panic("Runqueue bitmap indicates ready task but list is empty");
            } else if (!is_empty && !bit_set) {
                 kernel_panic("Runqueue list has tasks but bitmap bit is cleared");
            }
        }
    }
#else
    (void)rq;
#endif
}
