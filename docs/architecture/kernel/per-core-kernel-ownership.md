# Per-Core Kernel Ownership Contract

## Status
Phase K0 baseline

## Goal
Define what each CPU owns and how remote CPUs interact safely.

## Core Rule
No core directly mutates another core's scheduler, PMM, timer, or local IPC state.

## Per-Core Owned State
- **Scheduler Runqueue**: Each core owns its `sched_rq_t` structure.
- **Current Thread**: The currently executing thread on a core.
- **Idle Thread**: A dedicated idle thread per core.
- **Local PMM Cache**: (Planned for K2) Per-core page magazines.
- **Pending Scheduler Command Queue**: A typed inbox for remote operations.
- **Current Address-Space Tracking**: The active `address_space_t` on the CPU.
- **Per-Core Counters**: Performance and debug counters (e.g., context switches, IPIs).

## Remote Operation Rule
Remote actions must go through typed command queues and generation validation. Direct mutation of remote runqueues is prohibited.

### Protocol
1. **Source Core**: Enqueues a `sched_remote_cmd_t` into the target core's `pending_inbox`.
2. **Source Core**: Sends an IPI to the target core if no reschedule is already pending.
3. **Target Core**: Drains the inbox during `sched_reschedule`.
4. **Target Core**: Validates the thread generation ID to prevent stale operations.
5. **Target Core**: Updates its own local state (e.g., enqueues thread to local runqueue).

## Scheduler Invariants
1. **Single Runnable Owner**: A thread has exactly one runnable owner at any time.
2. **Explicit Enqueued State**: The `enqueued` flag must match actual queue membership.
3. **Owner State Consistency**: `owner_state` must correctly reflect whether a thread is running, enqueued, or blocked.

## Test Requirements
- Host-side stress tests for enqueue/dequeue cycles.
- Remote wake/migration protocol validation.
- Invariant violation detection (panics in debug builds).
