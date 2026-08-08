#include "sched/sched.h"
#include "sched_internal.h"
#include "panic.h"

void arch_post_switch(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  hal_cpu_enable_interrupts();
}

static void sched_remote_respond(const sched_remote_cmd_envelope_t *env, uint8_t kind, int32_t result) {
    sched_remote_respond_cell(env->handle.origin_cpu, env->handle.slot, env->handle.generation, kind, result);
}

static inline sched_entity_t *sched_find_entity_by_tid_local(sched_rq_t *rq, uint64_t tid) {
  for (size_t i = 0; i < SCHED_MAX_LOCAL_ENTITIES; ++i) {
    if (rq->entities[i].in_use && rq->entities[i].entity.tid == tid) {
      return &rq->entities[i].entity;
    }
  }
  return NULL;
}

static kstatus_t sched_validate_remote_envelope(uint32_t current_cpu, const sched_remote_cmd_envelope_t *env) {
    if (env->target_cpu != current_cpu)
        return K_ERR_INVALID_ARG;
    if (env->handle.origin_cpu >= g_active_core_count)
        return K_ERR_INVALID_ARG;
    if (env->handle.slot >= SCHED_REMOTE_CMD_CAPACITY)
        return K_ERR_INVALID_ARG;
    if (env->handle.generation == 0)
        return K_ERR_INVALID_ARG;
    if (env->source_cpu != env->handle.origin_cpu)
        return K_ERR_INVALID_ARG;
    return K_OK;
}

static kstatus_t sched_handle_migrate_reserve(uint32_t current_cpu, sched_rq_t *rq, const sched_remote_cmd_envelope_t *env) {
    sched_entity_t *entity = NULL;

    spin_lock(&rq->lock);
    entity = sched_find_entity_by_tid_local(rq, env->thread_id);
    if (entity) {
        if (entity->migration_epoch == env->migration_epoch) {
            spin_unlock(&rq->lock);
            ptrdiff_t diff = (sched_entity_slot_t *)((char *)entity - offsetof(sched_entity_slot_t, entity)) - rq->entities;
            sched_owner_locator_t loc = {
                .cpu = (uint16_t)current_cpu,
                .slot = (uint16_t)diff,
                .entity_generation = entity->entity_generation,
                .migration_epoch = entity->migration_epoch
            };
            sched_completion_publish(env->handle.origin_cpu, env->handle.slot, env->handle.generation, (uint16_t)current_cpu, SCHED_COMPLETION_ACK, (int32_t)diff, entity->migration_epoch, &loc, sizeof(loc));
            sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_ACK, (int32_t)diff);
            return K_OK;
        }
        spin_unlock(&rq->lock);
        sched_remote_respond(env, SCHED_COMPLETION_NACK, -5); // Conflict
        sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_NACK, -5);
        return K_OK;
    }

    entity = sched_allocate_entity(current_cpu);
    if (!entity) {
        spin_unlock(&rq->lock);
        sched_remote_respond(env, SCHED_COMPLETION_NACK, -19); // Out of resources
        sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_NACK, -19);
        return K_OK;
    }

    entity->tid = env->thread_id;
    entity->state = THREAD_STATE_READY;
    entity->priority = env->priority;
    entity->vruntime = env->vruntime;
    entity->absolute_deadline = env->absolute_deadline;
    entity->rt_attr = env->rt_attr;
    entity->context = env->context;
    entity->migration_state = SCHED_MIG_TARGET_RESERVED;
    entity->migration_epoch = env->migration_epoch;
    entity->runnable = false;
    entity->is_on_runqueue = 0;

    spin_unlock(&rq->lock);

    ptrdiff_t diff = (sched_entity_slot_t *)((char *)entity - offsetof(sched_entity_slot_t, entity)) - rq->entities;
    sched_owner_locator_t loc = {
        .cpu = (uint16_t)current_cpu,
        .slot = (uint16_t)diff,
        .entity_generation = entity->entity_generation,
        .migration_epoch = entity->migration_epoch
    };
    sched_completion_publish(env->handle.origin_cpu, env->handle.slot, env->handle.generation, (uint16_t)current_cpu, SCHED_COMPLETION_ACK, (int32_t)diff, entity->migration_epoch, &loc, sizeof(loc));
    sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_ACK, (int32_t)diff);
    return K_OK;
}

static kstatus_t sched_handle_migrate_stage(uint32_t current_cpu, sched_rq_t *rq, const sched_remote_cmd_envelope_t *env) {
    (void)current_cpu;
    spin_lock(&rq->lock);
    sched_entity_t *entity = sched_find_entity_by_tid_local(rq, env->thread_id);
    if (entity && entity->migration_epoch == env->migration_epoch) {
        if (entity->migration_state == SCHED_MIG_TARGET_RESERVED) {
            entity->vruntime = env->vruntime;
            entity->absolute_deadline = env->absolute_deadline;
            entity->rt_attr = env->rt_attr;
            entity->context = env->context;
            entity->migration_state = SCHED_MIG_TARGET_PREPARED;
        }
        spin_unlock(&rq->lock);
        sched_remote_respond(env, SCHED_COMPLETION_ACK, 0);
        sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_ACK, 0);
        return K_OK;
    }
    spin_unlock(&rq->lock);
    sched_remote_respond(env, SCHED_COMPLETION_NACK, -3);
    sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_NACK, -3);
    return K_OK;
}

static kstatus_t sched_handle_migrate_commit_identity(uint32_t current_cpu, sched_rq_t *rq, const sched_remote_cmd_envelope_t *env) {
    (void)rq;
    bh_thread_t *thread = sched_find_thread_by_id(env->thread_id);
    if (!thread) {
        sched_remote_respond(env, SCHED_COMPLETION_NACK, -1);
        return K_OK;
    }

    if (thread->migration_epoch < env->migration_epoch) {
        thread->owner_cpu = env->target_cpu;
        thread->owner_locator.cpu = (uint16_t)env->target_cpu;
        thread->owner_locator.slot = env->target_entity_slot;
        thread->owner_locator.entity_generation = env->target_entity_generation;
        thread->owner_locator.migration_epoch = env->migration_epoch;
        thread->migration_epoch = env->migration_epoch;

        sched_remote_respond(env, SCHED_COMPLETION_ACK, 0);
        sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_ACK, 0);
        return K_OK;
    } else if (thread->migration_epoch == env->migration_epoch && thread->owner_cpu == env->target_cpu) {
        sched_remote_respond(env, SCHED_COMPLETION_ACK, 0);
        sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_ACK, 0);
        return K_OK;
    } else {
        sched_remote_respond(env, SCHED_COMPLETION_NACK, -2); // Conflict
        sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_NACK, -2);
        return K_OK;
    }
}

static kstatus_t sched_handle_migrate_activate(uint32_t current_cpu, sched_rq_t *rq, const sched_remote_cmd_envelope_t *env) {
    spin_lock(&rq->lock);
    sched_entity_t *entity = sched_find_entity_by_tid_local(rq, env->thread_id);
    if (entity) {
        if (entity->migration_state == SCHED_MIG_TARGET_RESERVED) {
            entity->migration_state = SCHED_MIG_NONE;
            entity->runnable = true;

            bh_thread_t *thread = sched_find_thread_by_id(env->thread_id);
            if (thread) {
                sched_invariant_on_enqueue(thread, current_cpu);
                if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
                    sched_cfs_enqueue(rq, thread);
                } else {
                    list_add(&entity->run_node, &rq->ready_queue[entity->priority]);
                    sched_ready_bitmap_set(rq, entity->priority);
                }
                entity->is_on_runqueue = 1;
                rq->runnable_count++;
            }
        }
        spin_unlock(&rq->lock);
        sched_remote_respond(env, SCHED_COMPLETION_ACK, 0);
        sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_ACK, 0);
        return K_OK;
    } else {
        for (size_t i = 0; i < SCHED_MAX_LOCAL_ENTITIES; ++i) {
            if (rq->entities[i].in_use && rq->entities[i].entity.tid == env->thread_id && rq->entities[i].entity.runnable) {
                spin_unlock(&rq->lock);
                sched_remote_respond(env, SCHED_COMPLETION_ACK, 0);
                sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_ACK, 0);
                return K_OK;
            }
        }
        spin_unlock(&rq->lock);
        sched_remote_respond(env, SCHED_COMPLETION_NACK, -3);
        sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_NACK, -3);
        return K_OK;
    }
}

static kstatus_t sched_handle_query_state(uint32_t current_cpu, sched_rq_t *rq, const sched_remote_cmd_envelope_t *env) {
    (void)current_cpu;
    spin_lock(&rq->lock);
    sched_entity_t *entity = sched_find_entity_by_tid_local(rq, env->thread_id);
    int32_t result_code = 0; // ABSENT
    if (entity) {
        if (entity->runnable) {
            result_code = 1; // ACTIVE
        } else {
            result_code = 2; // RESERVED
        }
    }
    spin_unlock(&rq->lock);
    sched_remote_respond(env, SCHED_COMPLETION_ACK, result_code);
    sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_ACK, result_code);
    return K_OK;
}

static kstatus_t sched_handle_remote_wake(uint32_t current_cpu, sched_rq_t *rq, const sched_remote_cmd_envelope_t *env) {
    bh_thread_t *thread = sched_find_thread_by_id(env->thread_id);
    if (!thread) {
        sched_remote_respond(env, SCHED_COMPLETION_NACK, -1);
        return K_OK;
    }

    spin_lock(&rq->lock);
    sched_entity_t *entity = sched_find_entity_by_thread(thread);
    if (!entity) {
        spin_unlock(&rq->lock);
        sched_remote_respond(env, SCHED_COMPLETION_NACK, -4);
        return K_OK;
    }

    if (env->priority <= SCHED_MAX_PRIORITY && env->priority > entity->priority) {
        entity->priority = env->priority;
    }

    if (entity->state == THREAD_STATE_SLEEPING || entity->state == THREAD_STATE_BLOCKED) {
        entity->state = THREAD_STATE_READY;
        entity->runnable = true;

        thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
        if (slot) {
            if (slot->is_sleeping) sched_sleep_dequeue(slot);
            if (slot->is_blocked) sched_block_dequeue(slot);
        }

        sched_invariant_on_enqueue(thread, current_cpu);
        if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
            sched_cfs_enqueue(rq, thread);
        } else {
            list_add(&entity->run_node, &rq->ready_queue[entity->priority]);
            sched_ready_bitmap_set(rq, entity->priority);
        }
        entity->is_on_runqueue = 1;
        rq->runnable_count++;
    }

    spin_unlock(&rq->lock);
    sched_remote_respond(env, SCHED_COMPLETION_ACK, 0);
    sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_ACK, 0);
    return K_OK;
}

static kstatus_t sched_handle_set_priority(uint32_t current_cpu, sched_rq_t *rq, const sched_remote_cmd_envelope_t *env) {
    (void)current_cpu;
    bh_thread_t *thread = sched_find_thread_by_id(env->thread_id);
    if (!thread) {
        sched_remote_respond(env, SCHED_COMPLETION_NACK, -1);
        return K_OK;
    }

    spin_lock(&rq->lock);
    sched_entity_t *entity = sched_find_entity_by_thread(thread);
    if (entity) {
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
        entity->priority = env->priority;
        if (entity->state == THREAD_STATE_READY) {
            sched_invariant_on_enqueue(thread, current_cpu);
            if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
                sched_cfs_enqueue(rq, thread);
            } else if (g_policy == SCHED_POLICY_EDF) {
                sched_edf_enqueue(rq, thread);
            } else {
                list_add(&entity->run_node, &rq->ready_queue[entity->priority]);
                sched_ready_bitmap_set(rq, entity->priority);
            }
            entity->is_on_runqueue = 1U;
            rq->runnable_count++;
        }
    }
    spin_unlock(&rq->lock);
    sched_remote_respond(env, SCHED_COMPLETION_ACK, 0);
    sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_ACK, 0);
    return K_OK;
}

static kstatus_t sched_handle_set_affinity(uint32_t current_cpu, sched_rq_t *rq, const sched_remote_cmd_envelope_t *env) {
    (void)current_cpu;
    bh_thread_t *thread = sched_find_thread_by_id(env->thread_id);
    if (thread) {
        spin_lock(&rq->lock);
        sched_entity_t *entity = sched_find_entity_by_thread(thread);
        if (entity) {
            entity->affinity_mask = env->flags;
        }
        spin_unlock(&rq->lock);
    }
    sched_remote_respond(env, SCHED_COMPLETION_ACK, 0);
    sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_ACK, 0);
    return K_OK;
}

static kstatus_t sched_handle_set_constraints(uint32_t current_cpu, sched_rq_t *rq, const sched_remote_cmd_envelope_t *env) {
    (void)current_cpu;
    bh_thread_t *thread = sched_find_thread_by_id(env->thread_id);
    if (thread) {
        spin_lock(&rq->lock);
        sched_entity_t *entity = sched_find_entity_by_thread(thread);
        if (entity) {
            entity->constraints = env->constraints;
        }
        spin_unlock(&rq->lock);
    }
    sched_remote_respond(env, SCHED_COMPLETION_ACK, 0);
    sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_ACK, 0);
    return K_OK;
}

static kstatus_t sched_handle_quarantine(uint32_t current_cpu, sched_rq_t *rq, const sched_remote_cmd_envelope_t *env) {
    (void)current_cpu;
    (void)rq;
    bh_thread_t *thread = sched_find_thread_by_id(env->thread_id);
    if (thread) {
        sched_quarantine_thread(thread, env->flags);
    }
    sched_remote_respond(env, SCHED_COMPLETION_ACK, 0);
    sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_ACK, 0);
    return K_OK;
}

static kstatus_t sched_handle_terminate(uint32_t current_cpu, sched_rq_t *rq, const sched_remote_cmd_envelope_t *env) {
    (void)current_cpu;
    (void)rq;
    bh_thread_t *thread = sched_find_thread_by_id(env->thread_id);
    if (thread) {
        sched_mark_thread_terminated(thread);
    }
    sched_remote_respond(env, SCHED_COMPLETION_ACK, 0);
    sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_ACK, 0);
    return K_OK;
}

static kstatus_t sched_handle_reap(uint32_t current_cpu, sched_rq_t *rq, const sched_remote_cmd_envelope_t *env) {
    (void)current_cpu;
    (void)rq;
    bh_thread_t *thread = sched_find_thread_by_id(env->thread_id);
    if (thread) {
        thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
        if (slot) {
            sched_enqueue_reap(slot);
        }
    }
    sched_remote_respond(env, SCHED_COMPLETION_ACK, 0);
    sched_log_txn(rq, env->handle, env->thread_id, env->migration_epoch, env->type, SCHED_COMPLETION_ACK, 0);
    return K_OK;
}

void sched_reschedule(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  sched_remote_cmd_poll_timeouts();
  sched_reap_terminated_threads();
  sched_process_pending_ai_suggestions();

  hal_cpu_disable_interrupts(); // Fast path local lockless

  sched_rq_t *rq = &g_cpu_locals[core].runqueue;

  #define SCHED_REMOTE_DRAIN_BUDGET 64U
  uint32_t drained = 0;

  if (rq->remote.resched_pending != 0U || !sched_cmd_ring_empty(&rq->remote.cmd_ring)) {
      rq->remote.resched_pending = 0U;

      sched_remote_cmd_envelope_t envelope;
      while (drained < SCHED_REMOTE_DRAIN_BUDGET && sched_cmd_ring_pop(&rq->remote.cmd_ring, &envelope) == K_OK) {
          __atomic_fetch_add(&rq->remote.consumed, 1, __ATOMIC_RELAXED);
          drained++;

          if (sched_validate_remote_envelope(core, &envelope) != K_OK) {
              continue;
          }

          uint16_t cached_outcome = 0;
          int32_t cached_result = 0;
          if (sched_find_txn(rq, envelope.handle, &cached_outcome, &cached_result)) {
              sched_remote_respond(&envelope, cached_outcome, cached_result);
              continue;
          }

          if (envelope.type == SCHED_REMOTE_MIGRATE_RESERVE) {
              sched_handle_migrate_reserve(core, rq, &envelope);
          } else if (envelope.type == SCHED_REMOTE_MIGRATE_STAGE) {
              sched_handle_migrate_stage(core, rq, &envelope);
          } else if (envelope.type == SCHED_REMOTE_MIGRATE_COMMIT_IDENTITY) {
              sched_handle_migrate_commit_identity(core, rq, &envelope);
          } else if (envelope.type == SCHED_REMOTE_MIGRATE_ACTIVATE) {
              sched_handle_migrate_activate(core, rq, &envelope);
          } else if (envelope.type == SCHED_REMOTE_QUERY_STATE) {
              sched_handle_query_state(core, rq, &envelope);
          } else if (envelope.type == SCHED_REMOTE_WAKE) {
              sched_handle_remote_wake(core, rq, &envelope);
          } else if (envelope.type == SCHED_REMOTE_SET_PRIORITY) {
              sched_handle_set_priority(core, rq, &envelope);
          } else if (envelope.type == SCHED_REMOTE_SET_AFFINITY) {
              sched_handle_set_affinity(core, rq, &envelope);
          } else if (envelope.type == SCHED_REMOTE_SET_CONSTRAINTS) {
              sched_handle_set_constraints(core, rq, &envelope);
          } else if (envelope.type == SCHED_REMOTE_QUARANTINE) {
              sched_handle_quarantine(core, rq, &envelope);
          } else if (envelope.type == SCHED_REMOTE_TERMINATE) {
              sched_handle_terminate(core, rq, &envelope);
          } else if (envelope.type == SCHED_REMOTE_REAP) {
              sched_handle_reap(core, rq, &envelope);
          } else if (envelope.type == SCHED_REMOTE_MIGRATE_PREPARE) {
              bh_thread_t *thread = sched_find_thread_by_id(envelope.thread_id);
              if (thread) {
                  sched_migrate_task(thread, envelope.target_cpu);
              }
              sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              sched_log_txn(rq, envelope.handle, envelope.thread_id, envelope.migration_epoch, envelope.type, SCHED_COMPLETION_ACK, 0);
          } else if (envelope.type == SCHED_REMOTE_STEAL_REQ) {
              spin_lock(&rq->lock);
              bh_thread_t *victim = sched_find_steal_candidate(core, envelope.target_cpu);
              if (victim) {
                  sched_entity_t *v_entity = sched_find_entity_by_thread(victim);
                  if (v_entity && v_entity->is_on_runqueue != 0U) {
                      sched_invariant_on_dequeue(victim);
                      if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
                          sched_cfs_dequeue(rq, victim);
                      } else {
                          list_del(&v_entity->run_node);
                          list_init(&v_entity->run_node);
                          sched_ready_bitmap_clear_if_empty(rq, victim->priority);
                      }
                      v_entity->is_on_runqueue = 0U;
                      rq->runnable_count--;
                  }
              }
              spin_unlock(&rq->lock);
              sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              sched_log_txn(rq, envelope.handle, envelope.thread_id, envelope.migration_epoch, envelope.type, SCHED_COMPLETION_ACK, 0);
          } else {
              sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              sched_log_txn(rq, envelope.handle, envelope.thread_id, envelope.migration_epoch, envelope.type, SCHED_COMPLETION_ACK, 0);
          }
      }

      if (!sched_cmd_ring_empty(&rq->remote.cmd_ring)) {
          rq->remote.resched_pending = 1;
      }

      sched_publish_load(rq);
  }

  if (g_cpu_locals[core].runqueue.throttled != 0U && g_cpu_locals[core].runqueue.idle_thread) {
    sched_publish_load(rq);
    sched_switch_to(g_cpu_locals[core].runqueue.idle_thread, core);
    return;
  }

  bh_thread_t *next = sched_pick_next_ready(core);
  sched_publish_load(rq);
  sched_switch_to(next, core);
}

void sched_on_timer_tick(void) {
  sched_remote_cmd_poll_timeouts();
  g_cpu_locals[sched_clamp_core(hal_cpu_get_id())].runqueue.total_ticks++;


  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  sched_publish_load(&g_cpu_locals[core].runqueue);

  ipc_async_check_timeouts(g_cpu_locals[core].runqueue.total_ticks);

  list_head_t *sleep_head = &g_cpu_locals[core].runqueue.sleeping_list;
  list_head_t *curr = sleep_head->next;
  while (curr != sleep_head) {
    thread_slot_t *slot = (thread_slot_t *)(void *)((char *)curr - offsetof(thread_slot_t, wait_node));
    curr = curr->next;
    if (slot->thread.state == THREAD_STATE_SLEEPING &&
        slot->thread.wake_deadline_ms <= g_cpu_locals[core].runqueue.total_ticks) {
      sched_wake_tid(slot->thread.thread_id);
    }
  }

  list_head_t *block_head = &g_cpu_locals[core].runqueue.blocked_list;
  curr = block_head->next;
  while (curr != block_head) {
    thread_slot_t *slot = (thread_slot_t *)(void *)((char *)curr - offsetof(thread_slot_t, wait_node));
    curr = curr->next;
    if (slot->thread.state == THREAD_STATE_BLOCKED &&
        slot->thread.ipc_deadline_ticks > 0 &&
        slot->thread.ipc_deadline_ticks <= g_cpu_locals[core].runqueue.total_ticks) {
      slot->thread.ipc_wakeup_reason = -3; // IPC_ERR_WOULD_BLOCK or TIMEOUT
      slot->thread.ipc_deadline_ticks = 0;

      // Unlink it from wait queues handled by endpoint access so we can awaken it
      slot->thread.next_waiter = NULL;
      sched_wake_tid(slot->thread.thread_id);
    }
  }

  sched_process_pending_ai_suggestions();
  sched_reap_terminated_threads();

  if ((g_cpu_locals[core].runqueue.total_ticks % 16U) == 0U && core == 0U) {
    sched_balance_once();
  }

  sched_rq_t* rq = &g_cpu_locals[core].runqueue;
  bh_thread_t *current = rq->current_thread;
  if (!current) {
    sched_reschedule();
    return;
  }

  current->cpu_time_consumed++;

  if (g_policy == SCHED_POLICY_CLOUD_FAIR && current != rq->idle_thread) {
    sched_cfs_update_vruntime(rq, current, 1);
  }

  sched_update_telemetry(current);

  if (g_policy == SCHED_POLICY_EDF && current != rq->idle_thread) {
      if (current->cpu_time_consumed >= current->rt_attr.wcet_ms) {
          // Task exhausted budget for this period, wait for next period
          current->absolute_deadline_ms += current->rt_attr.period_ms;
          current->cpu_time_consumed = 0U;

          thread_slot_t *slot = sched_find_thread_slot_by_tid(current->thread_id);
          if (slot) {
              // Suspend the thread until the start of the next period
              current->wake_deadline_ms = current->absolute_deadline_ms - current->rt_attr.deadline_ms;
              current->state = THREAD_STATE_SLEEPING;
              sched_sleep_enqueue(slot, core);
              rq->current_thread = NULL;
          }
          sched_reschedule();
          return;
      }

      bh_thread_t *next = sched_edf_pick_next(rq);
      if (next && next->absolute_deadline_ms < current->absolute_deadline_ms) {
          sched_reschedule();
          return;
      }
  } else {
      if (current->cpu_time_consumed >= current->time_slice_ms) {
        current->cpu_time_consumed = 0U;
        sched_reschedule();
        return;
      }

      if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
          bh_thread_t *next = sched_cfs_pick_next(rq);
          if (next && next->vruntime < current->vruntime) {
              sched_reschedule();
              return;
          }
      } else {
          uint32_t higher_mask = (current->priority >= SCHED_MAX_PRIORITY)
                                     ? 0U
                                     : ((~0U) << (current->priority + 1U));
          if ((rq->ready_bitmap & higher_mask) != 0U) {
            sched_reschedule();
            return;
          }
      }
  }
}

sched_rq_t *sched_local_rq(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  return &g_cpu_locals[core].runqueue;
}

void sched_assert_local_rq(sched_rq_t *rq) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  if (rq != &g_cpu_locals[core].runqueue) {
    kernel_panic("sched_assert_local_rq failed: mutation of remote runqueue");
  }
}

sched_remote_cmd_t *sched_allocate_outbound_cmd(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[core].runqueue;
  uint32_t slot_idx = 0xFFFF;

  for (uint32_t w = 0; w < SCHED_CMD_BITMAP_WORDS; ++w) {
    uint32_t val = __atomic_load_n(&rq->outbound_alloc_bitmap[w], __ATOMIC_ACQUIRE);
    while (val != 0xFFFFFFFFU) {
      uint32_t free_bit = __builtin_ctz(~val);
      uint32_t new_val = val | (1U << free_bit);
      if (__atomic_compare_exchange_n(&rq->outbound_alloc_bitmap[w], &val, new_val, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
        slot_idx = w * 32 + free_bit;
        break;
      }
    }
    if (slot_idx != 0xFFFF) {
      break;
    }
  }

  if (slot_idx >= SCHED_REMOTE_CMD_CAPACITY) {
    return NULL;
  }

  sched_remote_cmd_t *cmd = &rq->outbound_cmds[slot_idx];

  // Increment generation (start of RESERVED state)
  cmd->handle.generation++;
  if (cmd->handle.generation == 0) {
    cmd->handle.generation = 1;
  }

  sched_completion_arm(rq, slot_idx, cmd->handle.generation);

  cmd->cmd_id = slot_idx;
  cmd->type = (sched_remote_cmd_type_t)0;
  cmd->source_cpu = core;
  cmd->target_cpu = 0;
  cmd->thread_id = 0;
  cmd->expected_thread_generation = 0;
  cmd->flags = 0;
  cmd->priority = 0;
  cmd->migration_epoch = 0;
  cmd->result = 0;
  cmd->submit_tick = 0;
  cmd->deadline_tick = 0;
  cmd->target_entity_slot = 0;
  cmd->target_entity_generation = 0;
  cmd->vruntime = 0;
  cmd->absolute_deadline = 0;
  __builtin_memset(&cmd->context, 0, sizeof(cmd->context));
  __builtin_memset(&cmd->rt_attr, 0, sizeof(cmd->rt_attr));
  list_init(&cmd->list);

  // Set state to RESERVED under memory barrier
  __atomic_store_n(&cmd->state, SCHED_REMOTE_CMD_RESERVED, __ATOMIC_RELEASE);

  return cmd;
}

void sched_remote_cmd_release(sched_remote_cmd_t *cmd) {
  if (!cmd) return;
  uint16_t slot = cmd->handle.slot;
  uint32_t w = slot / 32;
  uint32_t bit = slot % 32;
  uint32_t mask = ~(1U << bit);

  // Set state to EMPTY
  __atomic_store_n(&cmd->state, SCHED_REMOTE_CMD_EMPTY, __ATOMIC_RELEASE);

  // Clear from bitmap
  uint32_t core = cmd->handle.origin_cpu;
  sched_rq_t *rq = &g_cpu_locals[core].runqueue;
  __atomic_fetch_and(&rq->outbound_alloc_bitmap[w], mask, __ATOMIC_ACQ_REL);
}

kstatus_t sched_remote_submit(uint32_t target_cpu, const sched_remote_cmd_t *cmd) {
  if (target_cpu >= g_active_core_count) {
    return K_ERR_INVALID_ARG;
  }
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  if (target_cpu == current_core) {
    return K_ERR_INVALID_ARG;
  }

  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;
  sched_rq_t *target_rq = &g_cpu_locals[target_cpu].runqueue;
  sched_remote_cmd_t *mutable_cmd = (sched_remote_cmd_t *)cmd;

  mutable_cmd->submit_tick = rq->total_ticks;
  mutable_cmd->deadline_tick = rq->total_ticks + 10U; // 10 ticks deadline
  __atomic_store_n(&mutable_cmd->state, SCHED_REMOTE_CMD_PENDING, __ATOMIC_RELEASE);

  sched_remote_cmd_envelope_t envelope;
  envelope.handle = cmd->handle;
  envelope.type = cmd->type;
  envelope.source_cpu = cmd->source_cpu;
  envelope.target_cpu = cmd->target_cpu;
  envelope.thread_id = cmd->thread_id;
  envelope.expected_thread_generation = cmd->expected_thread_generation;
  envelope.migration_epoch = cmd->migration_epoch;
  envelope.flags = cmd->flags;
  envelope.priority = cmd->priority;
  envelope.constraints = cmd->constraints;
  envelope.context = cmd->context;
  envelope.vruntime = cmd->vruntime;
  envelope.absolute_deadline = cmd->absolute_deadline;
  envelope.rt_attr = cmd->rt_attr;
  envelope.target_entity_slot = cmd->target_entity_slot;
  envelope.target_entity_generation = cmd->target_entity_generation;

  kstatus_t status = sched_cmd_ring_push(&target_rq->remote.cmd_ring, &envelope);
  if (status != K_OK) {
    __atomic_store_n(&mutable_cmd->state, SCHED_REMOTE_CMD_RESERVED, __ATOMIC_RELEASE);
    __atomic_fetch_add(&target_rq->remote.full, 1, __ATOMIC_RELAXED);
    return K_ERR_NO_RESOURCES;
  }

  __atomic_fetch_add(&target_rq->remote.submitted, 1, __ATOMIC_RELAXED);

  uint32_t expected = 0;
  if (__atomic_compare_exchange_n(&target_rq->remote.resched_pending, &expected, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
      __atomic_fetch_add(&target_rq->remote.ipi_sent, 1, __ATOMIC_RELAXED);
      uint64_t msg = MK_MSG_THREAD_ENQUEUE_REQ;
      if (cmd->type == SCHED_REMOTE_WAKE) {
          msg = MK_MSG_THREAD_WAKE_REQ;
      } else if (cmd->type == SCHED_REMOTE_MIGRATE || cmd->type == SCHED_REMOTE_MIGRATE_PREPARE) {
          msg = MK_MSG_THREAD_DEQUEUE_REQ;
      }
      hal_send_ipi_payload(1U << target_cpu, msg);
  } else {
      __atomic_fetch_add(&target_rq->remote.ipi_coalesced, 1, __ATOMIC_RELAXED);
  }

  return K_OK;
}

kstatus_t sched_read_load_snapshot(uint32_t cpu, sched_load_snapshot_t *out) {
  if (cpu >= g_active_core_count || !out) return K_ERR_INVALID_ARG;
  sched_rq_t *rq = &g_cpu_locals[cpu].runqueue;
  uint32_t seq;
  do {
    seq = __atomic_load_n(&rq->load_snapshot.load_seq, __ATOMIC_ACQUIRE);
    out->runnable_count = __atomic_load_n(&rq->load_snapshot.runnable_count, __ATOMIC_ACQUIRE);
  } while ((seq & 1) != 0 || seq != __atomic_load_n(&rq->load_snapshot.load_seq, __ATOMIC_ACQUIRE));
  out->load_seq = seq;
  return K_OK;
}

bool sched_read_isolated_snapshot(uint32_t cpu) {
  if (cpu >= g_active_core_count) return false;
  sched_rq_t *rq = &g_cpu_locals[cpu].runqueue;
  return __atomic_load_n(&rq->sched_isolated, __ATOMIC_ACQUIRE);
}

kstatus_t sched_migration_transition(bh_thread_t *thread, sched_migration_state_t expected, sched_migration_state_t next) {
  if (!thread) return K_ERR_INVALID_ARG;
  uint32_t state = __atomic_load_n(&thread->migration_state, __ATOMIC_ACQUIRE);
  if (state != (uint32_t)expected) {
#if !defined(NDEBUG)
    kernel_panic("sched_migration_transition failed");
#else
    // Production safety fallback: reject transition, quarantine if ambiguous
    if (expected == SCHED_MIGRATION_ROLLBACK_SENT || next == SCHED_MIGRATION_FAILED) {
        sched_quarantine_thread(thread, THREAD_FAULT_MIGRATION_ROLLBACK_FAILED);
    }
    return K_ERR_BAD_STATE;
#endif
  }
  __atomic_store_n(&thread->migration_state, (uint32_t)next, __ATOMIC_RELEASE);
  return K_OK;
}

void sched_remote_cmd_poll_timeouts(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[core].runqueue;
  uint64_t current_ticks = rq->total_ticks;

  // 1. Drain completions from the completions cells
  for (uint32_t i = 0; i < SCHED_REMOTE_CMD_CAPACITY; ++i) {
    sched_remote_cmd_t *cmd = &rq->outbound_cmds[i];
    uint32_t cmd_state = __atomic_load_n(&cmd->state, __ATOMIC_ACQUIRE);

    if (cmd_state == SCHED_REMOTE_CMD_PENDING) {
      sched_completion_cell_t *cell = &rq->completions[i];
      uint32_t cell_state = __atomic_load_n(&cell->state, __ATOMIC_ACQUIRE);

      if (cell_state != SCHED_REMOTE_CMD_EMPTY) {
        if (cell->generation == cmd->handle.generation) {
          uint32_t next_state = cell_state;
          cmd->result = cell->result;

          __atomic_store_n(&cell->state, SCHED_REMOTE_CMD_EMPTY, __ATOMIC_RELEASE);
          __atomic_store_n(&cmd->state, next_state, __ATOMIC_RELEASE);
        }
      }
    }
  }

  // 2. Process deadlines and transition to TIMEOUT
  for (uint32_t i = 0; i < SCHED_REMOTE_CMD_CAPACITY; ++i) {
    sched_remote_cmd_t *cmd = &rq->outbound_cmds[i];
    uint32_t state = __atomic_load_n(&cmd->state, __ATOMIC_ACQUIRE);

    if (state == SCHED_REMOTE_CMD_PENDING) {
      if (cmd->deadline_tick > 0 && current_ticks >= cmd->deadline_tick) {
        __atomic_store_n(&cmd->state, SCHED_REMOTE_CMD_TIMEOUT, __ATOMIC_RELEASE);
      }
    }
  }

  // 3. Process actions on finalized/terminal commands
  for (uint32_t i = 0; i < SCHED_REMOTE_CMD_CAPACITY; ++i) {
    sched_remote_cmd_t *cmd = &rq->outbound_cmds[i];
    uint32_t state = __atomic_load_n(&cmd->state, __ATOMIC_ACQUIRE);

    if (state == SCHED_REMOTE_CMD_ACKED || state == SCHED_REMOTE_CMD_FAILED || state == SCHED_REMOTE_CMD_TIMEOUT) {
      bh_thread_t *thread = sched_find_thread_by_id(cmd->thread_id);
      if (thread) {
        if (thread->sched_generation == cmd->expected_thread_generation &&
            thread->migration_epoch == cmd->migration_epoch) {

          sched_entity_t *entity = sched_find_entity_by_thread(thread);

          if (cmd->type == SCHED_REMOTE_MIGRATE_RESERVE) {
            if (state == SCHED_REMOTE_CMD_ACKED) {
              cmd->target_entity_slot = (uint16_t)cmd->result;
              cmd->target_entity_generation = 1;

              if (entity) {
                entity->migration_state = SCHED_MIG_SOURCE_FROZEN;
                entity->runnable = false;
                if (entity->run_node.next && entity->run_node.next != &entity->run_node) {
                  list_del(&entity->run_node);
                  list_init(&entity->run_node);
                }
              }
              sched_migration_transition(thread, SCHED_MIG_RESERVE_SENT, SCHED_MIG_SOURCE_FROZEN);

              sched_remote_cmd_t *stage_cmd = sched_allocate_outbound_cmd();
              if (stage_cmd) {
                stage_cmd->type = SCHED_REMOTE_MIGRATE_STAGE;
                stage_cmd->source_cpu = core;
                stage_cmd->target_cpu = thread->migration_target_cpu;
                stage_cmd->thread_id = thread->thread_id;
                stage_cmd->expected_thread_generation = thread->sched_generation;
                stage_cmd->migration_epoch = thread->migration_epoch;
                stage_cmd->target_entity_slot = cmd->target_entity_slot;
                stage_cmd->target_entity_generation = cmd->target_entity_generation;
                if (entity) {
                  stage_cmd->vruntime = entity->vruntime;
                  stage_cmd->absolute_deadline = entity->absolute_deadline;
                  stage_cmd->rt_attr = entity->rt_attr;
                  stage_cmd->context = entity->context;
                }
                stage_cmd->state = SCHED_REMOTE_CMD_PENDING;

                sched_migration_transition(thread, SCHED_MIG_SOURCE_FROZEN, SCHED_MIG_TARGET_PREPARED);
                if (entity) entity->migration_state = SCHED_MIG_TARGET_PREPARED;

                kstatus_t status = sched_remote_submit(thread->migration_target_cpu, stage_cmd);
                if (status != K_OK) {
                  sched_remote_cmd_release(stage_cmd);
                  if (entity) {
                    entity->migration_state = SCHED_MIG_NONE;
                    entity->runnable = true;
                    sched_enqueue(thread, core);
                  }
                  sched_migration_transition(thread, SCHED_MIG_TARGET_PREPARED, SCHED_MIG_FAILED);
                }
              } else {
                if (entity) {
                  entity->migration_state = SCHED_MIG_NONE;
                  entity->runnable = true;
                  sched_enqueue(thread, core);
                }
                sched_migration_transition(thread, SCHED_MIG_SOURCE_FROZEN, SCHED_MIG_FAILED);
              }
            } else {
              if (entity) entity->migration_state = SCHED_MIG_NONE;
              sched_migration_transition(thread, SCHED_MIG_RESERVE_SENT, SCHED_MIG_NONE);
            }
          }

          else if (cmd->type == SCHED_REMOTE_MIGRATE_STAGE) {
            if (state == SCHED_REMOTE_CMD_ACKED) {
              sched_remote_cmd_t *commit_cmd = sched_allocate_outbound_cmd();
              if (commit_cmd) {
                commit_cmd->type = SCHED_REMOTE_MIGRATE_COMMIT_IDENTITY;
                commit_cmd->source_cpu = core;
                commit_cmd->target_cpu = thread->migration_target_cpu;
                commit_cmd->thread_id = thread->thread_id;
                commit_cmd->expected_thread_generation = thread->sched_generation;
                commit_cmd->migration_epoch = thread->migration_epoch;
                commit_cmd->target_entity_slot = cmd->target_entity_slot;
                commit_cmd->target_entity_generation = cmd->target_entity_generation;
                commit_cmd->state = SCHED_REMOTE_CMD_PENDING;

                sched_migration_transition(thread, SCHED_MIG_TARGET_PREPARED, SCHED_MIG_OWNER_COMMIT_SENT);
                if (entity) entity->migration_state = SCHED_MIG_OWNER_COMMIT_SENT;

                kstatus_t status = sched_remote_submit(bh_tid_identity_home_cpu(thread->thread_id), commit_cmd);
                if (status != K_OK) {
                  sched_remote_cmd_release(commit_cmd);
                  if (entity) {
                    entity->migration_state = SCHED_MIG_NONE;
                    entity->runnable = true;
                    sched_enqueue(thread, core);
                  }
                  sched_migration_transition(thread, SCHED_MIG_OWNER_COMMIT_SENT, SCHED_MIG_FAILED);
                }
              } else {
                if (entity) {
                  entity->migration_state = SCHED_MIG_NONE;
                  entity->runnable = true;
                  sched_enqueue(thread, core);
                }
                sched_migration_transition(thread, SCHED_MIG_TARGET_PREPARED, SCHED_MIG_FAILED);
              }
            } else {
              if (entity) {
                entity->migration_state = SCHED_MIG_NONE;
                entity->runnable = true;
                sched_enqueue(thread, core);
              }
              sched_migration_transition(thread, SCHED_MIG_TARGET_PREPARED, SCHED_MIG_NONE);
            }
          }

          else if (cmd->type == SCHED_REMOTE_MIGRATE_COMMIT_IDENTITY) {
            if (state == SCHED_REMOTE_CMD_ACKED) {
              sched_remote_cmd_t *activate_cmd = sched_allocate_outbound_cmd();
              if (activate_cmd) {
                activate_cmd->type = SCHED_REMOTE_MIGRATE_ACTIVATE;
                activate_cmd->source_cpu = core;
                activate_cmd->target_cpu = thread->migration_target_cpu;
                activate_cmd->thread_id = thread->thread_id;
                activate_cmd->expected_thread_generation = thread->sched_generation;
                activate_cmd->migration_epoch = thread->migration_epoch;
                activate_cmd->state = SCHED_REMOTE_CMD_PENDING;

                sched_migration_transition(thread, SCHED_MIG_OWNER_COMMIT_SENT, SCHED_MIG_ACTIVATE_SENT);
                if (entity) entity->migration_state = SCHED_MIG_ACTIVATE_SENT;

                kstatus_t status = sched_remote_submit(thread->migration_target_cpu, activate_cmd);
                if (status != K_OK) {
                  sched_remote_cmd_release(activate_cmd);
                  sched_quarantine_thread(thread, THREAD_FAULT_MIGRATION_ROLLBACK_FAILED);
                  sched_migration_transition(thread, SCHED_MIG_ACTIVATE_SENT, SCHED_MIG_FAILED);
                }
              } else {
                sched_quarantine_thread(thread, THREAD_FAULT_MIGRATION_ROLLBACK_FAILED);
                sched_migration_transition(thread, SCHED_MIG_OWNER_COMMIT_SENT, SCHED_MIG_FAILED);
              }
            } else {
              if (entity) {
                entity->migration_state = SCHED_MIG_NONE;
                entity->runnable = true;
                sched_enqueue(thread, core);
              }
              sched_migration_transition(thread, SCHED_MIG_OWNER_COMMIT_SENT, SCHED_MIG_NONE);
            }
          }

          else if (cmd->type == SCHED_REMOTE_MIGRATE_ACTIVATE) {
            if (state == SCHED_REMOTE_CMD_ACKED) {
              if (entity) {
                sched_free_entity(core, entity);
              }
              sched_migration_transition(thread, SCHED_MIG_ACTIVATE_SENT, SCHED_MIG_NONE);
            } else {
              sched_remote_cmd_t *query_cmd = sched_allocate_outbound_cmd();
              if (query_cmd) {
                query_cmd->type = SCHED_REMOTE_QUERY_STATE;
                query_cmd->source_cpu = core;
                query_cmd->target_cpu = thread->migration_target_cpu;
                query_cmd->thread_id = thread->thread_id;
                query_cmd->expected_thread_generation = thread->sched_generation;
                query_cmd->migration_epoch = thread->migration_epoch;
                query_cmd->state = SCHED_REMOTE_CMD_PENDING;

                sched_migration_transition(thread, SCHED_MIG_ACTIVATE_SENT, SCHED_MIG_RECONCILING);
                sched_remote_submit(thread->migration_target_cpu, query_cmd);
              } else {
                sched_quarantine_thread(thread, THREAD_FAULT_MIGRATION_ROLLBACK_FAILED);
                sched_migration_transition(thread, SCHED_MIG_ACTIVATE_SENT, SCHED_MIG_FAILED);
              }
            }
          }

          else if (cmd->type == SCHED_REMOTE_QUERY_STATE) {
            if (state == SCHED_REMOTE_CMD_ACKED && cmd->result == 1) {
              if (entity) {
                sched_free_entity(core, entity);
              }
              sched_migration_transition(thread, SCHED_MIG_RECONCILING, SCHED_MIG_NONE);
            } else {
              sched_quarantine_thread(thread, THREAD_FAULT_MIGRATION_ROLLBACK_FAILED);
              sched_migration_transition(thread, SCHED_MIG_RECONCILING, SCHED_MIG_FAILED);
            }
          }
        }
      }
      sched_remote_cmd_release(cmd);
    }
  }
}

kstatus_t sched_cmd_ring_init(sched_cmd_ring_t *q, sched_cmd_slot_t *slots, uint32_t capacity) {
    if (!q || !slots || capacity < 2 || (capacity & (capacity - 1)) != 0) {
        return K_ERR_INVALID_ARG;
    }
    q->slots = slots;
    q->capacity = capacity;
    q->mask = capacity - 1;
    q->head = 0;
    q->tail = 0;

    for (uint32_t i = 0; i < capacity; i++) {
        q->slots[i].seq = i;
        __builtin_memset(&q->slots[i].value, 0, sizeof(sched_remote_cmd_envelope_t));
    }
    return K_OK;
}

kstatus_t sched_cmd_ring_push(sched_cmd_ring_t *q, const sched_remote_cmd_envelope_t *value) {
    if (!q || !value) return K_ERR_INVALID_ARG;
    sched_cmd_slot_t *slot;
    uint64_t pos = q->head;

    while (true) {
        slot = &q->slots[pos & q->mask];
        uint64_t seq = __atomic_load_n(&slot->seq, __ATOMIC_ACQUIRE);
        int64_t diff = (int64_t)seq - (int64_t)pos;

        if (diff == 0) {
            if (__atomic_compare_exchange_n(&q->head, &pos, pos + 1, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
                break;
            }
        } else if (diff < 0) {
            return K_ERR_AGAIN;
        } else {
            pos = __atomic_load_n(&q->head, __ATOMIC_RELAXED);
        }
    }

    slot->value = *value;
    __atomic_store_n(&slot->seq, pos + 1, __ATOMIC_RELEASE);
    return K_OK;
}

kstatus_t sched_cmd_ring_pop(sched_cmd_ring_t *q, sched_remote_cmd_envelope_t *out_value) {
    if (!q) return K_ERR_INVALID_ARG;
    sched_cmd_slot_t *slot;
    uint64_t pos = q->tail;

    slot = &q->slots[pos & q->mask];
    uint64_t seq = __atomic_load_n(&slot->seq, __ATOMIC_ACQUIRE);
    int64_t diff = (int64_t)seq - (int64_t)(pos + 1);

    if (diff == 0) {
        q->tail = pos + 1;
        if (out_value) {
            *out_value = slot->value;
        }
        __atomic_store_n(&slot->seq, pos + q->mask + 1, __ATOMIC_RELEASE);
        return K_OK;
    }
    return K_ERR_AGAIN;
}

bool sched_cmd_ring_empty(const sched_cmd_ring_t *q) {
    if (!q) return true;
    uint64_t head = __atomic_load_n(&q->head, __ATOMIC_RELAXED);
    return q->tail == head;
}

kstatus_t sched_completion_ring_init(sched_completion_ring_t *q, sched_completion_slot_t *slots, uint32_t capacity) {
    if (!q || !slots || capacity < 2 || (capacity & (capacity - 1)) != 0) {
        return K_ERR_INVALID_ARG;
    }
    q->slots = slots;
    q->capacity = capacity;
    q->mask = capacity - 1;
    q->head = 0;
    q->tail = 0;

    for (uint32_t i = 0; i < capacity; i++) {
        q->slots[i].seq = i;
        __builtin_memset(&q->slots[i].value, 0, sizeof(sched_remote_completion_t));
    }
    return K_OK;
}

kstatus_t sched_completion_ring_push(sched_completion_ring_t *q, const sched_remote_completion_t *value) {
    if (!q || !value) return K_ERR_INVALID_ARG;
    sched_completion_slot_t *slot;
    uint64_t pos = q->head;

    while (true) {
        slot = &q->slots[pos & q->mask];
        uint64_t seq = __atomic_load_n(&slot->seq, __ATOMIC_ACQUIRE);
        int64_t diff = (int64_t)seq - (int64_t)pos;

        if (diff == 0) {
            if (__atomic_compare_exchange_n(&q->head, &pos, pos + 1, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
                break;
            }
        } else if (diff < 0) {
            return K_ERR_AGAIN;
        } else {
            pos = __atomic_load_n(&q->head, __ATOMIC_RELAXED);
        }
    }

    slot->value = *value;
    __atomic_store_n(&slot->seq, pos + 1, __ATOMIC_RELEASE);
    return K_OK;
}

kstatus_t sched_completion_ring_pop(sched_completion_ring_t *q, sched_remote_completion_t *out_value) {
    if (!q) return K_ERR_INVALID_ARG;
    sched_completion_slot_t *slot;
    uint64_t pos = q->tail;

    slot = &q->slots[pos & q->mask];
    uint64_t seq = __atomic_load_n(&slot->seq, __ATOMIC_ACQUIRE);
    int64_t diff = (int64_t)seq - (int64_t)(pos + 1);

    if (diff == 0) {
        q->tail = pos + 1;
        if (out_value) {
            *out_value = slot->value;
        }
        __atomic_store_n(&slot->seq, pos + q->mask + 1, __ATOMIC_RELEASE);
        return K_OK;
    }
    return K_ERR_AGAIN;
}

bool sched_completion_ring_empty(const sched_completion_ring_t *q) {
    if (!q) return true;
    uint64_t head = __atomic_load_n(&q->head, __ATOMIC_RELAXED);
    return q->tail == head;
}
