import re

file_path = "kernel/src/sched/sched.c"
with open(file_path, 'r') as f:
    content = f.read()

# Fix memory leaks by checking if arrays exist
new_reset = """void sched_reset_core_runqueues(void) {
  for (uint32_t core = 0; core < g_active_core_count; ++core) {
    sched_rq_t *rq = &g_cpu_locals[core].runqueue;
    rq->current_thread = NULL;
    rq->idle_thread = NULL;
    rq->total_ticks = 0U;
    rq->context_switches = 0U;
    rq->runnable_count = 0U;
    rq->throttled = 0U;
    rq->resched_pending = 0U;
    rq->remote_enqueues = 0U;
    rq->ipi_sent = 0U;
    rq->ipi_coalesced = 0U;
    rq->inbox_drains = 0U;
    rq->remote_preemptions = 0U;
    spin_lock_init(&rq->lock);
    list_init(&rq->sleeping_list);
    list_init(&rq->blocked_list);
    list_init(&rq->pending_inbox);
    for (uint32_t p = 0; p < MAX_PRIORITY_LEVELS; ++p) {
      list_init(&rq->ready_queue[p]);
    }
    rq->ready_bitmap = 0U;
    rq->edf_runqueue.rb_node = NULL;
    rq->rt_budget_used = 0U;
    rq->rt_budget_total = 0U;

    rq->reap_head = UINT32_MAX;
    rq->reap_tail = UINT32_MAX;

    if (!rq->threads) rq->threads = (struct thread_slot*)kmalloc(sizeof(thread_slot_t) * SCHED_MAX_THREADS);
    if (!rq->processes) rq->processes = (struct process_slot*)kmalloc(sizeof(process_slot_t) * SCHED_MAX_PROCESSES);
    if (!rq->bootstrap_threads) rq->bootstrap_threads = (struct thread_slot*)kmalloc(sizeof(thread_slot_t) * SCHED_BOOTSTRAP_THREAD_TYPES);
    if (!rq->bootstrap_stacks) rq->bootstrap_stacks = (uint8_t*)kmalloc(16384U * SCHED_BOOTSTRAP_THREAD_TYPES);
    if (!rq->mutex_owners) rq->mutex_owners = kmalloc(sizeof(mutex_owner_entry_t) * SCHED_MAX_THREADS);
    if (!rq->pending_suggestions) rq->pending_suggestions = kmalloc(sizeof(suggestion_queue_t));

    if (!rq->threads || !rq->processes || !rq->bootstrap_threads || !rq->bootstrap_stacks || !rq->mutex_owners || !rq->pending_suggestions) {
        kernel_panic("sched_reset_core_runqueues kmalloc failed");
    }

    rq->free_thread_head = 0U;
    for (size_t i = 0; i < SCHED_MAX_THREADS; ++i) {
        ((thread_slot_t*)rq->threads)[i].in_use = 0U;
        ((thread_slot_t*)rq->threads)[i].is_bootstrap = 0U;
        ((thread_slot_t*)rq->threads)[i].next_free = (i + 1U < SCHED_MAX_THREADS) ? (uint32_t)(i + 1U) : UINT32_MAX;
        ((thread_slot_t*)rq->threads)[i].reap_next = UINT32_MAX;
        ((thread_slot_t*)rq->threads)[i].reap_pending = 0U;
    }

    rq->free_process_head = 0U;
    for (size_t i = 0; i < SCHED_MAX_PROCESSES; ++i) {
        ((process_slot_t*)rq->processes)[i].in_use = 0U;
        ((process_slot_t*)rq->processes)[i].next_free = (i + 1U < SCHED_MAX_PROCESSES) ? (uint32_t)(i + 1U) : UINT32_MAX;
    }

    memset(rq->bootstrap_threads, 0, sizeof(thread_slot_t) * SCHED_BOOTSTRAP_THREAD_TYPES);
    memset(rq->mutex_owners, 0, sizeof(mutex_owner_entry_t) * SCHED_MAX_THREADS);
    memset(rq->pending_suggestions, 0, sizeof(suggestion_queue_t));
  }
}
"""
content = re.sub(r"void sched_reset_core_runqueues\(void\) \{[\s\S]*?memset\(rq->pending_suggestions, 0, sizeof\(suggestion_queue_t\)\);\n  \}\n\}", new_reset, content)

with open(file_path, 'w') as f:
    f.write(content)
