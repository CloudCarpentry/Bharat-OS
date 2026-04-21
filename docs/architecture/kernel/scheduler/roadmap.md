# Scheduler Roadmap

## Status snapshot (from current scheduler code)

### Stable baseline

- Per-core runqueue model with remote enqueue inbox.
- Priority scheduling path with bitmap/list invariants.
- CFS and EDF queue structures integrated.
- EDF and RMS admission checks with per-core utilization budgets.
- Tick-driven wakeup/preemption and periodic balance hook.
- AI hint ingestion queue with bounded pending buffer.

### Under active hardening

- Full router-contract migration across all scheduler mutation paths.
- Cross-core balancing heuristics (affinity/topology/NUMA sensitivity).
- Observability for cross-core handoff and AI action outcomes.
- Extended stress/fuzz tests for policy transitions and queue pressure.

## Next milestones

1. Finish router-boundary refactor and document ownership preconditions for each mutation path.
2. Expand scheduler host test matrix for policy-specific invariants (priority/CFS/EDF/RMS).
3. Add structured trace and reason-code surfaces for AI action acceptance/rejection.
4. Improve migration quality with partition/topology-aware constraints.

## Longer horizon

- Stronger mixed-criticality validation with explicit partition contracts.
- Profile-tuned scheduling presets and conformance suites.
- Multikernel domain-level scheduling coordination contracts.
