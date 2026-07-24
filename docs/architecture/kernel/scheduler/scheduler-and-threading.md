---
title: Scheduler and Threading Baseline
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# Scheduler and Threading Baseline

This document reflects the scheduler/threading baseline from current kernel code under `core/kernel/src/sched/` and `core/kernel/include/sched/`.

## Core data model

`bh_thread_t` and `sched_rq_t` define the runtime contract:

- Thread state machine (`THREAD_STATE_*`) with ready/running/blocked/sleeping/terminated plus distributed handoff states.
- Per-core runqueue with:
  - priority queues + bitmap,
  - CFS rb-tree,
  - EDF rb-tree,
  - sleeping/blocked lists,
  - pending inbox for remote enqueue,
  - per-core counters (ticks/context switches/runnable depth).

## Implemented scheduler mechanisms

- Policy selection: RR, cloud-fair, priority, EDF, RMS.
- Enqueue/dequeue with ownership and runnable accounting.
- Timer tick processing:
  - sleep wakeups,
  - IPC timeout wakeups,
  - AI suggestion processing,
  - periodic balancing,
  - preemption decision.
- Cross-core handoff/migration paths.
- Priority inheritance helper hooks.
- Reaper queue for terminated threads.

## Real-time behavior (implemented)

- EDF admission checks and per-core RT utilization tracking.
- RMS admission checks with static-priority mapping.
- EDF runtime behavior in tick handler includes budget exhaustion handling and next-period sleep.

## Architecture coupling

The scheduler remains architecture-neutral at API level and depends on HAL/arch hooks for timer, core-id, interrupts, and context switch primitives.

## Test coverage (current)

Scheduler behavior is validated in both host and core/kernel/user-level tests, including:

- scheduler functional tests (`quality/tests/test_scheduler.c`, `quality/tests/host/test_sched.c`),
- scheduler partition validation (`quality/tests/host/test_sched_partition_validation.c`),
- scheduler benchmarks (`quality/tests/test_bench_sched*.c`, `quality/tests/benchmark/suites/scheduler/bench_sched.c`),
- kernel selftests (`core/kernel/src/quality/tests/ktest_sched*.c`).

## Known gaps to harden

- Router contract adoption is still incomplete across all paths.
- Some policy docs still reference planned behavior beyond current implementation depth.
- Cross-core balancing and migration heuristics need deeper topology-aware tuning.
