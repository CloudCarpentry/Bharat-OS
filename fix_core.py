import re

file_path = "kernel/src/sched/sched.c"
with open(file_path, 'r') as f:
    content = f.read()

# Fix the out of bounds index calculation and use the original creation core for the thread pool instead of target core
new_tcd = """  slot->thread.thread_id = g_next_thread_id++;
  slot->thread.process_id = parent ? parent->process_id : 0U;
  slot->thread.process = parent;
  slot->thread.personality = PERSONALITY_NATIVE;
  slot->thread.state = THREAD_STATE_READY;
  slot->thread.priority = 1U;
  slot->thread.base_priority = 1U;
  slot->thread.cpu_time_consumed = 0U;
  slot->thread.time_slice_ms = SCHED_DEFAULT_SLICE_MS;
  slot->thread.preferred_numa_node = 0U;
  slot->thread.bound_core_id = sched_clamp_core(hal_cpu_get_id());
  slot->thread.affinity_mask = SCHED_AFFINITY_ANY;
  slot->thread.wake_deadline_ms = 0U;
  slot->thread.context_switch_count = 0U;
  slot->creation_core_id = sched_clamp_core(hal_cpu_get_id());"""

content = content.replace("  slot->thread.thread_id = __atomic_fetch_add((uint64_t*)&g_next_thread_id, 1U, __ATOMIC_SEQ_CST);\n  slot->thread.process_id = parent ? parent->process_id : 0U;\n  slot->thread.process = parent;\n  slot->thread.personality = PERSONALITY_NATIVE;\n  slot->thread.state = THREAD_STATE_READY;\n  slot->thread.priority = 1U;\n  slot->thread.base_priority = 1U;\n  slot->thread.cpu_time_consumed = 0U;\n  slot->thread.time_slice_ms = SCHED_DEFAULT_SLICE_MS;\n  slot->thread.preferred_numa_node = 0U;\n  slot->thread.bound_core_id = sched_clamp_core(hal_cpu_get_id());\n  slot->thread.affinity_mask = SCHED_AFFINITY_ANY;\n  slot->thread.wake_deadline_ms = 0U;\n  slot->thread.context_switch_count = 0U;", new_tcd)

content = content.replace("slot->thread.thread_id = __atomic_fetch_add((uint64_t*)&g_next_thread_id, 1U, __ATOMIC_SEQ_CST);", "slot->thread.thread_id = g_next_thread_id++;")
content = content.replace("slot->process.process_id = __atomic_fetch_add((uint64_t*)&g_next_process_id, 1U, __ATOMIC_SEQ_CST);", "slot->process.process_id = g_next_process_id++;")
content = content.replace("__atomic_fetch_add((uint64_t*)&g_sched_ticks, 1U, __ATOMIC_SEQ_CST);", "g_sched_ticks++;")
content = content.replace("__atomic_fetch_add((uint64_t*)&g_sched_context_switches, 1U, __ATOMIC_SEQ_CST);", "g_sched_context_switches++;")
content = content.replace("__atomic_store_n((uint64_t*)&g_next_thread_id, 1U, __ATOMIC_SEQ_CST);", "g_next_thread_id = 1U;")
content = content.replace("__atomic_store_n((uint64_t*)&g_next_process_id, 1U, __ATOMIC_SEQ_CST);", "g_next_process_id = 1U;")
content = content.replace("__atomic_store_n((uint64_t*)&g_sched_ticks, 0U, __ATOMIC_SEQ_CST);", "g_sched_ticks = 0U;")
content = content.replace("__atomic_store_n((uint64_t*)&g_sched_context_switches, 0U, __ATOMIC_SEQ_CST);", "g_sched_context_switches = 0U;")
content = content.replace("__atomic_load_n((uint64_t*)&g_sched_ticks, __ATOMIC_SEQ_CST)", "g_sched_ticks")


new_td = """int thread_destroy(kthread_t *thread) {
  // Reaper-only contract:
  // - call only from deferred reap context, never inline on the running thread.
  // - never destroy while executing on the victim thread's stack.
  if (!thread) {
    return -1;
  }
  if (thread == sched_current_thread()) {
    return -1;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
  if (!slot) {
    return -1;
  }

  uint32_t creation_core = sched_clamp_core(slot->creation_core_id);
  sched_rq_t *rq = &g_cpu_locals[creation_core].runqueue;

  // Prevent race with background reaper or other cores
  hal_cpu_disable_interrupts();

  spin_lock(&rq->lock);

  if (slot->reap_pending && slot->reap_next != UINT32_MAX) {
      // Thread is already being reaped, let the reaper finish it
      spin_unlock(&rq->lock);
      hal_cpu_enable_interrupts();
      return 0;
  }

  slot->reap_pending = 1U; // Mark as pending so reaper doesn't touch it
  spin_unlock(&rq->lock);

  sched_detach_thread_from_queues(slot);

  // Clean up architecture extended state
  arch_ext_state_thread_destroy(thread);

  if (thread->capability_list) {
    cap_table_destroy(thread->capability_list);
    thread->capability_list = NULL;
  }

  if (thread->kernel_stack) {
    void *stack_ptr = (void*)(uintptr_t)thread->kernel_stack;
    thread->kernel_stack = 0U; // Clear first to prevent double-free if re-entered
    kfree(stack_ptr);
  }

  // Final slot reclamation
  spin_lock(&rq->lock);
  slot->in_use = 0U;
  slot->reap_pending = 0U;

  if (!slot->is_bootstrap) {
    uint32_t idx = (uint32_t)(slot - (thread_slot_t*)rq->threads);
    slot->next_free = rq->free_thread_head;
    rq->free_thread_head = idx;
  }
  spin_unlock(&rq->lock);

  hal_cpu_enable_interrupts();
  return 0;
}
"""
content = re.sub(r"int thread_destroy\(kthread_t \*thread\) \{[\s\S]*?return 0;\n\}\n", new_td, content)


new_reap = """static int sched_enqueue_reap(thread_slot_t *slot) {
  if (!slot || slot->is_bootstrap != 0U) {
    return -1;
  }

  uint32_t core_id = sched_clamp_core(slot->creation_core_id);
  sched_rq_t *rq = &g_cpu_locals[core_id].runqueue;

  spin_lock(&rq->lock);
  if (slot->reap_pending != 0U) {
    spin_unlock(&rq->lock);
    return 0;
  }

  uint32_t idx = (uint32_t)(slot - (thread_slot_t*)rq->threads);
  slot->reap_pending = 1U;
  slot->reap_next = UINT32_MAX;
  if (rq->reap_tail == UINT32_MAX) {
    rq->reap_head = idx;
    rq->reap_tail = idx;
  } else {
    ((thread_slot_t*)rq->threads)[rq->reap_tail].reap_next = idx;
    rq->reap_tail = idx;
  }
  spin_unlock(&rq->lock);
  return 0;
}
"""
content = re.sub(r"static int sched_enqueue_reap\(thread_slot_t \*slot\) \{[\s\S]*?return 0;\n\}\n", new_reap, content)


with open(file_path, 'w') as f:
    f.write(content)

file_path = "kernel/src/sched/sched_internal.h"
with open(file_path, 'r') as f:
    content = f.read()

content = content.replace("  uint8_t is_blocked;\n} thread_slot_t;", "  uint8_t is_blocked;\n  uint32_t creation_core_id;\n} thread_slot_t;")

with open(file_path, 'w') as f:
    f.write(content)
