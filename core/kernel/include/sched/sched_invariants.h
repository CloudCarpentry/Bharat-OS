#ifndef BHARAT_SCHED_INVARIANTS_H
#define BHARAT_SCHED_INVARIANTS_H

/*
 * Scheduler invariant identifiers for documentation, tracing, and test mapping.
 *
 * Keeping these symbols centralized helps tests and diagnostics stay aligned with
 * architecture docs while refactor phases progressively split scheduler code.
 */
typedef enum {
    SCHED_INV_SINGLE_RUNNABLE_OWNERSHIP = 0,
    SCHED_INV_SINGLE_WRITER_HOT_PATH,
    SCHED_INV_EXPLICIT_REMOTE_PATH,
    SCHED_INV_CORE_LIFECYCLE_AUTHORITY,
    SCHED_INV_BOUNDED_AI_ACTIONS,
    SCHED_INV_STRICT_RT_ISOLATION,
    SCHED_INV_COUNT
} sched_invariant_id_t;

struct bh_thread;
typedef struct bh_thread bh_thread_t;

void sched_invariant_check_thread(bh_thread_t *thread);
void sched_invariant_on_enqueue(bh_thread_t *thread, uint32_t core_id);
void sched_invariant_on_dequeue(bh_thread_t *thread);
void sched_invariant_on_switch(bh_thread_t *prev, bh_thread_t *next, uint32_t core_id);

void sched_invariant_check_thread_owner(bh_thread_t *thread, uint32_t expected_cpu);
void sched_invariant_check_runqueue_exclusive(bh_thread_t *thread);
void sched_invariant_check_remote_enqueue_path(bh_thread_t *thread);

#endif /* BHARAT_SCHED_INVARIANTS_H */
