# Scheduler and Threading Baseline (v1)

This document captures the current baseline implementation for scheduler/threading in the kernel.

## Implemented in this baseline

- Static in-kernel process/thread registries.
- Thread control block (TCB) metadata including:
  - architectural context pointer,
  - priority/base priority,
  - scheduling state,
  - capability list hook pointer,
  - time-slice accounting.
- Round-robin dispatch with timer-tick driven preemption (`sched_on_timer_tick`).
- Scheduler policy switch interface (`sched_set_policy`) with baseline RR behavior.
- Thread lifecycle operations:
  - `thread_create`, `thread_destroy`,
  - syscall-style wrappers `sched_sys_thread_create`, `sched_sys_thread_destroy`.
- Context-switch hook integration via `fv_secure_context_switch` when available.

## Deferred for production

- Architecture-specific register save/restore and user-mode transitions.
- Real per-core run queues and SMP load balancing.
- Hardware timer interrupt source integration (APIC/CLINT/PLIC path).
- Full capability-list management and per-thread security context linkage.
