# Scheduler and Threading Baseline (v2)

This document captures the current scheduler/threading implementation status in the kernel.

## Implemented in v2

- Architecture-neutral Thread Control Block (`kthread`) in `kernel/include/sched.h` with:
  - architecture context pointer,
  - base + effective priority,
  - scheduling state,
  - capability list hook,
  - time-slice and context-switch accounting,
  - AI scheduler context,
  - NUMA hint + CPU affinity mask.
- Per-CPU scheduler state in `kernel/src/sched.c`:
  - per-core priority run queues,
  - per-core sleeping and blocked lists,
  - per-core idle thread,
  - per-core tick/context-switch counters.
- Thread lifecycle syscall-style entry points:
  - `sched_sys_thread_create`,
  - `sched_sys_thread_destroy`,
  - `sched_sys_sleep`,
  - `sched_sys_set_priority`,
  - `sched_sys_set_affinity`.
- Scheduling behavior:
  - priority-based round-robin dispatch,
  - timer tick preemption,
  - sleep/wakeup bookkeeping by deadline,
  - CPU affinity-aware migration,
  - basic work-stealing/load balancing pass.
- Priority inheritance hooks:
  - `sched_inherit_priority`,
  - `sched_restore_priority`.
- AI scheduling hook path:
  - bounded pending AI suggestion queue,
  - suggestion actions for reprioritize/migrate/throttle/kill,
  - telemetry collection during tick handling.
- Portable scheduler API surface:
  - `sched_current`, `sched_enqueue`, `sched_reschedule`, and existing thread APIs.

## Architecture-specific context switching hooks

- Common interface in `kernel/include/arch/context_switch.h`:
  - `arch_context_switch(cpu_context_t* prev, cpu_context_t* next)`
  - `arch_prepare_initial_context(cpu_context_t* ctx, void (*entry)(void), uint64_t stack_top)`
- Per-architecture implementations are now split under:
  - `kernel/src/arch/x86_64/context_switch.c`
  - `kernel/src/arch/arm64/context_switch.c`
  - `kernel/src/arch/riscv64/context_switch.c`
  - `kernel/src/arch/shakti/context_switch.c`

These files provide a consistent call signature so scheduler code remains architecture-neutral.

## Test and host validation

- Scheduler lifecycle, priority, sleep/wakeup, and affinity tests are covered by `tests/test_scheduler.c`.
- Host-executable architecture matrix validation script:
  - `tools/ci/run_scheduler_arch_matrix.sh`
  - builds/runs scheduler-oriented tests,
  - compiles each architecture context-switch source file from host.

## Remaining hardening items for production

- Replace context-switch C placeholders with full save/restore assembly for integer/FPU/vector state.
- Complete EDF/RMS admission/deadline accounting paths.
- Add lock-graph-aware transitive priority donation for nested mutex ownership chains.
- Expand NUMA-aware migration heuristics with topology distance/cost tables.
