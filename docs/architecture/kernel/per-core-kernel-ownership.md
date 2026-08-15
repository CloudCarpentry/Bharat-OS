---
title: Per-Core Kernel Ownership Contract
status: Active
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# Per-Core Kernel Ownership Contract

## Status
Active

## Implementation References
- `core/kernel/src/sched/sched_core.c`
- `core/kernel/include/sched/sched_invariants.h`
- `core/kernel/src/sched/sched_invariants.c`

## Goal
Define what each CPU owns and how remote CPUs interact safely.

## Core Rule
No core directly mutates another core's scheduler, PMM, timer, or local IPC state.

## Per-Core Owned State
- **Scheduler Runqueue**: Each core owns its `sched_rq_t` structure.
- **Current Thread**: The currently executing thread on a core.
- **Idle Thread**: A dedicated idle thread per core.
- **Local PMM Cache**: (PLANNED) Per-core page magazines.
- **Pending Scheduler Command Queue**: A typed command ring for remote operations (`remote_cmd_ring`).
- **Current Address-Space Tracking**: The active `address_space_t` on the CPU.
- **Per-Core Counters**: Performance and debug counters (e.g., context switches, IPIs).

## Remote Operation Rule
Remote actions must go through typed command rings and generation validation. Direct mutation of remote runqueues is prohibited.

### Protocol (IMPLEMENTED)
1. **Source Core**: Enqueues a `sched_cmd_t` into the target core's `remote_cmd_ring`.
2. **Source Core**: Sends an IPI (Core Notification) to the target core if no reschedule is already pending.
3. **Target Core**: Drains the ring.
4. **Target Core**: Validates the `thread_id` and generation to prevent stale operations.
5. **Target Core**: Updates its own local state (e.g., enqueues thread to local runqueue) and publishes a completion (ACK/NACK).

## Scheduler Invariants (IMPLEMENTED)
1. **Single Runnable Owner**: A thread has exactly one runnable owner at any time. Verified by `sched_invariant_check_runqueue_exclusive`.
2. **Explicit Enqueued State**: The `is_on_runqueue` flag must match actual queue membership.
3. **Owner State Consistency**: The thread state must correctly reflect whether a thread is running, enqueued, or blocked.

## Test Requirements
- Host-side stress tests for enqueue/dequeue cycles.
- Remote wake/migration protocol validation.
- Invariant violation detection (panics in debug builds).
