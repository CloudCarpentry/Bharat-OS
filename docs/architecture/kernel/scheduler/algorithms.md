# Scheduler Algorithms

## Active policy selector

The scheduler exposes `sched_set_policy(...)` with these policies:

- `SCHED_POLICY_ROUND_ROBIN`
- `SCHED_POLICY_CLOUD_FAIR`
- `SCHED_POLICY_PRIORITY`
- `SCHED_POLICY_EDF`
- `SCHED_POLICY_RMS`

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
