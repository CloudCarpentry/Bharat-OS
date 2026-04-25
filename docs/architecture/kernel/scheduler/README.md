---
title: Kernel Scheduler Documentation
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# Kernel Scheduler Documentation

This folder is the single source of truth for scheduler architecture, implementation status, and roadmap.

## Scope

The scheduler implementation lives primarily in:

- `core/kernel/src/sched/`
- `core/kernel/include/sched/`
- `quality/tests/test_scheduler.c`
- `quality/tests/host/test_sched.c`
- `quality/tests/host/test_sched_partition_validation.c`

## Current implementation shape (code-backed)

- Per-core runqueues with local ownership (`sched_rq_t`) and remote enqueue inbox support.
- Multiple policies selectable via `sched_set_policy(...)`: round-robin, cloud-fair, priority, EDF, RMS.
- Runnable ownership accounting (`runnable_count`, `ready_bitmap`, per-priority queues, CFS and EDF trees).
- Timer-tick driven preemption and wakeups (`sched_on_timer_tick`).
- RT admissions (`sched_admission_edf`, `sched_admission_rms`) with utilization budgets.
- AI suggestion ingestion and bounded application path (`sched_enqueue_ai_suggestion`, `sched_process_pending_ai_suggestions`).
- Cross-core migration and balancing primitives (`sched_migrate_task`, periodic `sched_balance_once`).

## Documents in this folder

- [Scheduler and Threading](scheduler-and-threading.md)
- [Scheduler Invariants and Host Tests](scheduler-invariants-and-host-tests.md)
- [AI Scheduler Overview](ai-scheduler-overview.md)
- [AI Scheduler Status and Roadmap](ai-scheduler-status-and-roadmap.md)
- [Algorithms](algorithms.md)
- [Runqueue](runqueue.md)
- [Priority](priority.md)
- [Time Accounting](time-accounting.md)
- [CPU Affinity](cpu-affinity.md)
- [Preemption](preemption.md)
- [Real-time](realtime.md)
- [Roadmap](roadmap.md)
- [Invariants and Router Contract](invariants-and-router-contract.md)
