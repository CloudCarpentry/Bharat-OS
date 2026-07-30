#include "sched/sched.h"
#include "sched_internal.h"

void sched_inherit_priority(bh_thread_t *thread, uint32_t new_priority) {
  if (!thread) {
    return;
  }
  if (new_priority > thread->priority) {
    thread->priority = new_priority;
  }
}

void sched_restore_priority(bh_thread_t *thread) {
  if (!thread) {
    return;
  }
  thread->priority = thread->base_priority;
}

void sched_on_mutex_wait(bh_thread_t *waiter, void *mutex) {
  if (!waiter || !mutex) {
    return;
  }

  waiter->waiting_on_lock = mutex;

  bh_thread_t *owner = sched_find_mutex_owner(mutex);
  while (owner && owner != waiter && waiter->priority > owner->priority) {
    sched_inherit_priority(owner, waiter->priority);
    if (!owner->waiting_on_lock) {
      break;
    }
    owner = sched_find_mutex_owner(owner->waiting_on_lock);
  }
}

void sched_on_mutex_acquire(bh_thread_t *owner, void *mutex) {
  if (!owner || !mutex) {
    return;
  }

  owner->waiting_on_lock = NULL;
  sched_register_mutex_owner(mutex, owner);
}

void sched_on_mutex_release(bh_thread_t *owner, void *mutex) {
  if (!owner || !mutex) {
    return;
  }

  sched_unregister_mutex_owner(mutex, owner);
  sched_restore_priority(owner);
}

int sched_adjust_priority_local(bh_thread_t *thread, uint32_t new_priority) {
  if (!thread) {
    return -1;
  }
  if (new_priority > SCHED_MAX_PRIORITY) {
    new_priority = SCHED_MAX_PRIORITY;
  }

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = sched_local_rq();

  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);

  if (slot && slot->is_on_runqueue != 0U) {
    hal_cpu_disable_interrupts();

    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
      sched_cfs_dequeue(rq, thread);
    } else {
      list_del(&slot->run_node);
      list_init(&slot->run_node);
      sched_ready_bitmap_clear_if_empty(rq, thread->priority);
    }

    slot->is_on_runqueue = 0U;
    if (rq->runnable_count > 0U) {
      rq->runnable_count--;
    }

    hal_cpu_enable_interrupts();
  }

  thread->priority = new_priority;
  if (thread->state == THREAD_STATE_READY) {
    (void)sched_enqueue(thread, current_core);
  }
  return 0;
}

int sched_set_priority(uint64_t tid, uint32_t priority) {
  bh_thread_t *thread = sched_find_thread_by_id(tid);
  if (!thread) return -1;

  if (priority > SCHED_MAX_PRIORITY) {
    priority = SCHED_MAX_PRIORITY;
  }

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  uint32_t owner = __atomic_load_n(&thread->owner_cpu, __ATOMIC_ACQUIRE);

  if (owner != current_core) {
      // Route command to owner_cpu.
      sched_remote_cmd_t *cmd = sched_allocate_outbound_cmd();
      if (!cmd) {
          return K_ERR_NO_RESOURCES;
      }
      cmd->type = SCHED_REMOTE_SET_PRIORITY;
      cmd->source_cpu = current_core;
      cmd->target_cpu = owner;
      cmd->thread_id = thread->thread_id;
      cmd->expected_thread_generation = thread->sched_generation;
      cmd->priority = priority;
      cmd->state = SCHED_REMOTE_CMD_PENDING;

      kstatus_t status = sched_remote_submit(owner, cmd);
      if (status != K_OK) {
          sched_remote_cmd_release(cmd);
          return status;
      }
      return 0;
  }

  return sched_adjust_priority_local(thread, priority);
}

int sched_set_thread_priority(uint64_t tid, uint32_t new_priority) {
  return sched_set_priority(tid, new_priority);
}

int sched_sys_set_priority(uint64_t tid, uint32_t new_priority) {
  return sched_set_priority(tid, new_priority);
}

