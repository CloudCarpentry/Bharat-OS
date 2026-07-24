# Execution Mode & CPU Partition Admission Contract

## Overview

Bharat-OS enforces a strict admission and partitioning contract for scheduler execution. This contract ensures that workload types (System, Realtime, Fair/GP) are routed to appropriate CPU cores based on the system's execution mode and hardware topology.

The goal is to provide predictable isolation for realtime tasks while maintaining general-purpose throughput where applicable, and supporting single-core temporal sharing for small devices.

## Partitioning Strategies

### 1. Temporal Partitioning (Single-Core)
Used when `active_cpu_count == 1`.
- All classes (SYSTEM, FIFO_RT, FAIR) coexist on CPU0.
- Separation is handled by scheduler class priority (RT wins over GP).
- No cross-core migration or remote routing.

### 2. Spatial Partitioning (Multi-Core)
Used when `active_cpu_count > 1`.
- Roles are assigned per CPU (SYSTEM, REALTIME, BEST_EFFORT, SPARE).
- Specific classes are admitted only on allowed CPUs.

## Execution Modes

### General Purpose (GP)
Focuses on broad throughput across all cores.
- All CPUs allow `SYSTEM | FAIR` classes.
- RT classes are typically rejected unless explicitly configured.

### Realtime (RT)
Focuses on strict RT isolation.
- `CPU0` is dedicated to `SYSTEM` tasks.
- `CPU1..N` are dedicated to `REALTIME` tasks.
- GP/FAIR tasks are generally rejected or routed to a `SPARE` core if available.

### Mixed-Criticality (MIX)
Balancing RT performance and GP throughput.
- Enforces separation of RT and FAIR workloads on multi-core systems.
- For 2 cores: `CPU0 = SYSTEM | RT`, `CPU1 = FAIR`.
- For 3+ cores: Explicitly separates `SYSTEM`, `RT`, and `FAIR` cores.

## Admission Rules by Core Count

| Mode | CPU Count | Strategy | CPU0 Role | CPU1 Role | CPU2 Role | CPU3..N Role |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Small** | 1 | Temporal | SYSTEM | - | - | - |
| **GP** | 2+ | Spatial | SYSTEM | SYSTEM | SYSTEM | SYSTEM |
| **RT** | 2 | Spatial | SYSTEM+RT | SPARE | - | - |
| **RT** | 3 | Spatial | SYSTEM | RT | SPARE | - |
| **RT** | 4+ | Spatial | SYSTEM | RT | RT | RT |
| **MIX** | 2 | Spatial | SYSTEM+RT | FAIR | - | - |
| **MIX** | 3 | Spatial | SYSTEM | RT | FAIR | - |
| **MIX** | 4+ | Spatial | SYSTEM | RT | RT | FAIR |

## Admission API

The admission layer provides deterministic helpers for the scheduler router:

- `sched_admission_allows_class(cpu_id, class_mask)`: Checks if a specific class is permitted on a CPU.
- `sched_admission_select_cpu(class_mask, out_cpu_id)`: Selects the first eligible CPU for a workload.
- `sched_admission_validate_partitions()`: Verifies that the current configuration meets invariant requirements (e.g., at least one SYSTEM CPU, no RT+FAIR collapse on multi-core).

## Invariants

1. **At least one SYSTEM CPU** must exist to ensure kernel health and housekeeping.
2. **Deterministic Selection**: In the absence of load balancing, CPU selection for admission must be deterministic (ascending CPU ID order).
3. **No RT+FAIR Collapse**: On systems with multiple cores in MIX mode, Realtime and Fair workloads must not be assigned to the same core to prevent GP interference with RT timing.
4. **Fail-Closed**: Invalid configurations (unknown modes, invalid CPU IDs, or violating invariants) must be rejected by the admission layer.
