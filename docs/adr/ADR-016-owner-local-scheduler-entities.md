---
title: "ADR-016: Owner-Local Scheduler Entities and Five-Phase Transactional Migration"
status: Accepted
owner: Kernel Working Group
last_updated: 2026-04-26
tags:
  - docs
  - adr
  - sched
see_also:
  - docs/architecture/000_KERNEL_ARCHITECTURE.md
  - docs/adr/ADR-010-distributed-kernel-ownership.md
---
# ADR-016: Owner-Local Scheduler Entities and Five-Phase Transactional Migration

## Status

Accepted

## Context

Prior versions of the Bharat-OS scheduler relied on thread structures (`bh_thread_t`) that resided on their home core's runqueue memory, even when the thread was migrated to and executed on a remote CPU. When another core attempted to wake, block, or adjust the priority of a migrated thread, it would follow the stable Thread ID (TID) to the home core's slot and mutate scheduling states.

This shared-state behavior violated the strict single-ownership invariants of Bharat-OS's multikernel design. It allowed cross-core mutations, created race conditions on runqueue lists and sched tree nodes (like CFS and EDF rb_nodes), and compromised SMP stress reliability under heavy load. To achieve absolute multikernel ownership closure, we need a complete separation between stable thread identity and core-local mutable scheduling state.

## Decision

We will transition the scheduler to a **Sovereign Owner-Local Execution Model**. This decision establishes a strict boundary between stable thread identity and owner-local execution, coordinated via a reliable transactional transport protocol.

### 1. Stable Identity vs. Execution Entity

We separate `bh_thread_t` (representing stable, immutable identity) from `sched_entity_t` (representing owner-local mutable execution state):

*   **Stable Thread Identity (`bh_thread_t`)**:
    *   Remains permanently allocated in the static `rq->threads` pool on the thread's original home CPU (the identity authority).
    *   The `TID` format `[generation:32 | identity_home_cpu:16 | identity_slot:16]` remains completely stable and never changes during migration.
    *   The `bh_tid_home_core(tid)` (and its new explicit alias `bh_tid_identity_home_cpu`) identifies the **identity authority**, not the execution owner.
    *   Contains process relationships, capability tables, process IDs, and stable lifecycle contexts.
*   **Owner-Local Execution Entity (`sched_entity_t`)**:
    *   Physically allocated from the current `owner_cpu`'s sovereign local `entities` pool.
    *   Holds all scheduling-mutable fields (priority, state, vruntime, absolute_deadline, rb_nodes, CPU register contexts, and scheduling-used affinity/constraints).
    *   No runqueue node, scheduler tree node, register context, or mutable scheduling state is ever shared or mutated remotely.

### 2. Owner Locator

Instead of cross-core raw pointers, `bh_thread_t` tracks its current location using an opaque, value-copied locator:

```c
typedef struct {
    uint16_t cpu;
    uint16_t slot;
    uint32_t entity_generation;
    uint32_t migration_epoch;
} sched_owner_locator_t;
```

This locator allows O(1) resolution from TID -> identity home CPU -> owner locator -> current owner CPU + local entity slot. Generation and epoch checks prevent any stale-owner resolving or ABA issues.

### 3. Five-Phase Transactional Migration

Migration transfers execution state strictly by value through a 5-phase transactional protocol, coordinated by Owner CPU A:

1.  **RESERVE (`MIGRATE_RESERVE`)**: CPU A sends a reservation request to Target CPU B. B allocates an empty slot in its local `entities` pool and transitions its state to `SCHED_MIG_TARGET_RESERVED`. B returns the locator details.
2.  **FREEZE**: CPU A receives the reservation details. It freezes the local source entity: state becomes `SCHED_MIG_SOURCE_FROZEN`, `runnable = false`, dequeued from A's ready queues (the slot is kept locked).
3.  **STAGE (`MIGRATE_STAGE`)**: CPU A builds an immutable migration image and sends it by value to B. B pre-installs the register context, vruntime, and deadlines on the reserved entity, transitioning its state to `SCHED_MIG_TARGET_PREPARED` (frozen but not runnable).
4.  **OWNER_COMMIT (`COMMIT_IDENTITY`)**: CPU A submits `COMMIT_IDENTITY` to the Thread's Home CPU C (identity authority). CPU C performs a CAS-like verification and updates the stable identity owner to B.
5.  **ACTIVATE (`MIGRATE_ACTIVATE`)**: CPU A sends `ACTIVATE` to B. B transitions the pre-installed entity to `SCHED_MIG_NONE` and enqueues it on its local ready queues.
6.  **RETIRE**: CPU A receives ACK for activation, frees the source entity slot on A, and releases the transaction.

### 4. Identity Commit Semantics

The TID's home CPU acts as the authoritative ownership serialization point. The `COMMIT_IDENTITY` command behaves like a distributed compare-and-swap (CAS), verifying that the old owner and epoch match exactly before committing the new owner locator. Duplicate identical commits are idempotent and safely ACKed, while conflicting ownership states fail closed.

### 5. Reliable Completion Transport

To prevent ring congestion and lost completions, we replace completion FIFOs with per-outbound-slot completion cells (`completions[SCHED_REMOTE_CMD_CAPACITY]`).
*   Every allocated outbound command slot owns its designated completion cell on the origin CPU.
*   The origin CPU arms the cell, publishes the remote command, and polls the cell.
*   The responder writes the ACK/NACK directly to the origin's cell with generation checks, preventing late ACKs from overwriting slot reuses.
*   This design guarantees zero-drop, ABA-safe completion delivery with bounded memory.

### 6. Timeout and Reconciliation

*   A timeout during migration is treated as `OUTCOME_UNKNOWN`, never assumed as a NACK or failure.
*   Timed out command slots remain reserved (preventing slot reuse) while the coordinator sends `QUERY_STATE` to reconcile.
*   `QUERY_STATE` can query B for entity state (`QUERY_ENTITY_STATE`) or C for identity locator (`QUERY_IDENTITY_OWNER`) to deterministically decide on commit finalization or rollback.

### 7. Idempotency

Command identity is verified via `(origin_cpu, slot, generation)` and additionally validated against the TID and migration epoch. The receiver circular transaction cache optimizes replay ACK responses, but the authoritative entity/identity states remain the ultimate source of truth.

### 8. Rollback

*   **Before OWNER_COMMIT**: A cancels B's reservation, unfreezes the source entity, and returns it to the local ready queues.
*   **After OWNER_COMMIT**: A cannot rollback unilaterally. It must first transactionally commit the identity back to A before unfreezing, ensuring that at no point can two runnable scheduler entities exist for the same TID.

### 9. Inbox-Processing Rules

*   Remote envelopes are fully validated (active CPUs, valid slots, correct targets) before indexing any CPU state.
*   Draining of remote commands is strictly bounded by a budget (e.g., 64) per reschedule, preventing remote traffic from creating unbounded scheduler latency.
*   Runqueue locks are acquired strictly around narrow local mutations, never held during completion publishing or remote command submissions.

## Consequences

### Positive

*   **Absolute Single-Ownership**: Mutual exclusion of ready list/tree nodes is physically guaranteed by local entity confinement.
*   **Predictable Concurrency**: Eliminated cross-core mutable pointer chases and locks.
*   **High SMP Reliability**: Staging and CAS-like commits eliminate double-runnable races and stale-owner wakeups.

### Negative

*   **Staging Overhead**: Copying context by value during STAGE introduces minor latency but yields complete memory isolation.

## Verification

The new architecture is fully verified by:
*   `test_sched_remote_lost_ack`: Verification of reserve/freeze/stage/commit/activate stages and deterministic lost ACK reconciliation.
*   `test_sched_remote_ownership`: Verifies absolute owner-local scheduling confinement.
*   `test_sched_remote_cmd` & `test_sched_remote_completion`: Verifies reliable, slot-designated completion cell delivery.
*   `check_scheduler_ownership.py`: Static linter verifying zero remote runqueue field writes.
*   `x86_64_desktop_headless` target build & QEMU self-tests.
