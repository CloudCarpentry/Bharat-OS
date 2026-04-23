#ifndef BHARAT_SCHED_ROUTER_CONTRACT_H
#define BHARAT_SCHED_ROUTER_CONTRACT_H

#include <stdint.h>
#include "sched/sched.h"

/*
 * Scheduler Router Contract
 *
 * This interface defines the narrow mechanism boundary between scheduler core
 * lifecycle/state ownership and scheduling-class specific implementations.
 *
 * Contract intent:
 * - keep core lifecycle and ownership invariants centralized,
 * - keep class-specific accounting local,
 * - avoid policy sprawl in core paths.
 */

/* Operation context hints for enqueue/dequeue/migrate pathways. */
typedef enum {
    SCHED_ROUTE_REASON_UNKNOWN = 0,
    SCHED_ROUTE_REASON_CREATE,
    SCHED_ROUTE_REASON_WAKE,
    SCHED_ROUTE_REASON_PREEMPT,
    SCHED_ROUTE_REASON_MIGRATE,
    SCHED_ROUTE_REASON_REMOTE_INBOX,
    SCHED_ROUTE_REASON_POLICY_CHANGE,
    SCHED_ROUTE_REASON_AI_HINT,
} sched_route_reason_t;

/*
 * Enqueue a thread through router-approved class/core paths.
 * Returns 0 on success, negative error code on rejection/failure.
 */
int sched_router_enqueue(bh_thread_t *thread, uint32_t core_id, sched_route_reason_t reason);

/*
 * Dequeue a thread through router-approved class/core paths.
 * Returns 0 on success, negative error code on rejection/failure.
 */
int sched_router_dequeue(bh_thread_t *thread, uint32_t core_id, sched_route_reason_t reason);

/*
 * Pick next runnable thread for the given core according to active composition.
 * Returns NULL when no runnable candidate exists (idle fallback expected).
 */
bh_thread_t *sched_router_pick_next(uint32_t core_id);

/*
 * Per-tick hook for router/class accounting updates.
 */
void sched_router_on_tick(uint32_t core_id, uint64_t now_ticks);

/*
 * Thread state transition hooks that preserve core lifecycle authority.
 */
void sched_router_on_block(bh_thread_t *thread, uint32_t core_id);
void sched_router_on_wake(bh_thread_t *thread, uint32_t core_id);

/*
 * Notify router about ownership transfer across cores.
 * Returns 0 on success, negative error code otherwise.
 */
int sched_router_on_migrate(bh_thread_t *thread,
                            uint32_t src_core,
                            uint32_t dst_core,
                            sched_route_reason_t reason);

#endif /* BHARAT_SCHED_ROUTER_CONTRACT_H */
