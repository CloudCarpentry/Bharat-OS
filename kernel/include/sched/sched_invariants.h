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

#endif /* BHARAT_SCHED_INVARIANTS_H */
