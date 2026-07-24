---
title: Context Switching Review: Kernel + Subsystem (revalidated)
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - reviews
see_also:
  - README.md
---
# Context Switching Review: Kernel + Subsystem (revalidated)

## Scope
- Kernel scheduler runqueue selection and context-switch path.
- Subsystem CPU allocation mask handling in `core/services/core/subsysmgr`.
- HAL topology query surface used by scheduling/allocation policy.

## Alignment Check Against Current Code

### 1) Priority selection and bitmap path
**Status: aligned (implemented).**
- Runqueue selection uses a priority bitmap and bit-scan helpers via `sched_pick_priority_from_bitmap()`.
- Highest-priority and lowest-priority paths are selected with `__builtin_clz` / `__builtin_ctz` (policy dependent), not linear priority scans.

### 2) Redundant self-switch accounting
**Status: aligned (implemented).**
- `sched_switch_to()` early-returns when `current == next`, so no context-switch counters or arch save/restore path are executed for a no-op switch.

### 3) Hardware-/telemetry-informed slice shaping
**Status: partially aligned.**
- AI telemetry and complexity signals are integrated in scheduler AI paths.
- Runtime usage exists, but end-to-end policy remains heuristic and not yet tied to deeper per-arch hardware pressure signals (cache miss, memory stalls, etc.).

### 4) Subsystem CPU mask normalization
**Status: aligned and improved.**
- Subsystem CPU masks are normalized in create/start paths.
- Updated to use HAL CPU-topology query so normalization is based on discovered CPU count rather than a fixed compile-time core cap in this module.

## Current Gaps (remaining)
1. **Lazy extended-context switching is still not implemented.**
   - Save/restore hooks exist, but no clear arch-specific lazy ownership strategy (e.g., XSAVE ownership tracking / trap-on-first-use model).
2. **Topology shape is still coarse for policy.**
   - Query returns discovered CPU count and basic SMP indicator, but no cluster/perf-class data for heterogeneous scheduling decisions.
3. **SMP scalability work remains open.**
   - Per-core queue locking exists, but full remote balancing/coalescing and explicit cross-core work-stealing policy is still an ongoing area.

## Next Code Task (selected)
### Task: **Topology-aware subsystem placement follow-through**
After replacing the fixed subsystem core limit with HAL topology query, the next task is:
1. Add per-subsystem allocation policy modes (packed/spread).
2. Use topology metadata (once extended) to map masks to performance or efficiency clusters.
3. Add tests for edge masks (`0`, out-of-range, full-mask) under different discovered CPU counts.

## Work started in this update
- Added `hal_cpu_topology_query()` implementation backed by discovered platform topology.
- Switched subsystem CPU mask normalization to use `hal_cpu_topology_query()` instead of local fixed `MAX_SUPPORTED_CORES`.
