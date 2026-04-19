# Scheduler Invariants and Router Contract

## Purpose

This document defines the non-negotiable scheduler invariants and the router-layer API contract that must remain valid during and after the `sched.c` decomposition.

It is intentionally mechanism-focused: policy tuning and orchestration should remain outside of the kernel scheduler core whenever possible.

## Core Invariants

### I1. Single runnable ownership
A runnable thread must belong to exactly one active scheduling ownership domain at a time:
- one core,
- one scheduling class queue,
- one enqueue representation.

A thread must never be present in multiple runqueues or class queues simultaneously.

### I2. Single-writer hot path
State mutations for runnable ownership (`enqueue`, `dequeue`, state transitions involving runnable placement) must follow a single-writer rule on the hot path.

Cross-core operations must not mutate a foreign core's runnable structures directly except through approved handoff/inbox mechanisms.

### I3. Explicit remote path
Remote enqueue, migration, and handoff must occur only through explicit cross-core mechanisms (inbox/message path) with clear ownership transfer points.

### I4. Core lifecycle authority
The scheduler core owns lifecycle state safety for runnable transitions and must remain authoritative for:
- legal thread-state transitions,
- enqueue/dequeue consistency,
- cross-class mutation boundaries.

Class modules may maintain class-local accounting but cannot bypass core state/lifecycle invariants.

### I5. Bounded AI actions
AI-originated actions are advisory hints and must be validated before mutation. They must not bypass normal enqueue/dequeue/state ownership rules.

Rejected AI actions must produce an observable reason code for auditability.

### I6. Strict RT mode isolation
Strict RT composition must not be forced into GP fairness behavior by global starvation logic. Any starvation guard must be a mixed-mode parameter, not a universal rule.

## Router Contract Surface

The router API is defined in `kernel/include/sched/sched_router_contract.h` and is the only cross-class mutation gateway.

Required operations:
- `sched_router_enqueue(...)`
- `sched_router_dequeue(...)`
- `sched_router_pick_next(...)`
- `sched_router_on_tick(...)`
- `sched_router_on_block(...)`
- `sched_router_on_wake(...)`
- `sched_router_on_migrate(...)`

## Contract Rules

1. **No hidden side effects:** each API must document permitted state mutation.
2. **Ownership preconditions:** each API must state lock/IRQ/ownership assumptions.
3. **Cross-core discipline:** cross-core mutation must delegate to explicit remote paths.
4. **Class boundary safety:** class logic cannot directly mutate foreign-class internals.
5. **Deterministic fallback:** router behavior must be deterministic when a class declines an operation.

## Review Gate for Refactor PRs

A scheduler decomposition PR is acceptable only if it preserves these invariants and does not weaken the contract boundary.

At minimum, each PR in the split sequence should include:
- invariants impact section,
- router contract impact section,
- ownership/cross-core path validation notes,
- tests that exercise affected invariants.
