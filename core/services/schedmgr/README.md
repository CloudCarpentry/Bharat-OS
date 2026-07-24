# services/schedmgr

## Purpose
A scheduling policy service above the kernel's dispatcher and context switcher.

## Responsibilities
- **Workload Classification**: Identifying and categorizing tasks.
- **Heterogeneous Placement**: Deciding placement across different CPU architectures (e.g., big.LITTLE).
- **Accelerator Affinity**: Ensuring workloads stay near relevant hardware (e.g., GPUs, NPUs).
- **Energy/Performance Balancing**: Throttling or migrating to optimize power and performance.
- **RT Admission Policy**: Deciding if real-time constraints can be met.
- **Work-Stealing Thresholds**: Deciding when idle cores should steal work.
- **Thermal-Aware Migration Policy**: Shifting loads to cooler domains when necessary.

*(Note: The kernel still owns dispatching, preemption, time accounting hooks, and remote wakeups.)*

## Dependencies
- **May depend on:** `lib/runtime`, `lib/ipc`, standard C library headers.
- **Must not depend on:** `subsys/*`, `ui/*`, direct kernel-private headers.

## Immediate TODOs
- Classify incoming threads and apply base affinity mappings.
- Integrate with `telemetrymgr` for power/thermal data.

## Status
Stub.
