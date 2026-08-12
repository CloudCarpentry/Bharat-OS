---
title: Scheduler Algorithms
status: Proposed
owner: Documentation Working Group
last_updated: 2026-08-12
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# Scheduler Algorithms

## Per-core policy selector

Each owner-local runqueue stores its own active policy. `sched_set_policy(...)`
changes only the calling core's policy, and `sched_get_policy()` reads only the
calling core's policy. There is no authoritative system-wide scheduler-policy
variable. This permits mixed compositions such as EDF on an RT partition and
cloud-fair scheduling on GP cores without a cross-core mutable policy lookup in
the dispatch hot path.

The available per-core policies are:

- `SCHED_POLICY_ROUND_ROBIN`
- `SCHED_POLICY_CLOUD_FAIR`
- `SCHED_POLICY_PRIORITY`
- `SCHED_POLICY_EDF`
- `SCHED_POLICY_RMS`

Policy state follows runqueue ownership: a core may mutate its own selector,
while remote placement and migration must use the bounded scheduler command
protocol and pass the destination core's partition/class admission checks.

## Priority/RR path

- Uses per-priority ready lists and `ready_bitmap`.
- Pick-next uses bitmap scanning for highest runnable priority.
- Timer tick enforces timeslice preemption.

## Cloud-fair path

- Uses CFS-style rb-tree (`cfs_runqueue`) and `vruntime` accounting.
- Tick updates `vruntime` for running non-idle thread.
- Preemption can occur when a lower-`vruntime` candidate is available.

## EDF path

- Uses deadline-ordered rb-tree (`edf_runqueue`).
- Admission is controlled by utilization budget (`rt_budget_used/rt_budget_total`).
- Tick path enforces WCET-per-period behavior and re-sleeps tasks for next period when budget is exhausted.

## RMS path

- Admission uses simplified RMS utilization bound.
- Priority is statically assigned from period buckets.
- Dispatch still reuses priority queue machinery.

## AI-assisted adjustments

AI suggestions do not replace class algorithms; they request bounded actions (migrate/priority/throttle/etc.) that are validated and applied by scheduler mechanisms.
