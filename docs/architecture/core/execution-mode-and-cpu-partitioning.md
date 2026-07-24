---
title: Execution Mode and CPU Partitioning Architecture
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - core
see_also:
  - README.md
---
# Execution Mode and CPU Partitioning Architecture

This document describes the design and rationale for the execution mode and CPU partitioning framework in Bharat-OS.

## Separation of "Profile" and "Execution Mode"

In previous iterations, the term "profile" was overloaded. This framework explicitly separates the two dimensions:
- **System Profile**: Defines the product or domain shape. Examples include `AUTOMOBILE`, `MOBILE`, `APPLIANCE`, and `DESKTOP`. It helps determine features such as UI presence, networking stacks, and standard peripherals.
- **Execution Mode**: Defines the compute and scheduling constraints. Examples include `REALTIME`, `GENERAL_PURPOSE`, and `MIXED_CRITICAL`.

By separating these, Bharat-OS can support combinations like an `AUTOMOBILE` profile running in `MIXED_CRITICAL` mode or an `APPLIANCE` profile running in `REALTIME` mode, without conflating product logic with scheduling mechanisms.

## CPU Partition Role vs Scheduler Class

The framework distinguishes between the role a CPU plays and the scheduling algorithm(s) it runs:
- **CPU Partition Role**: Describes the high-level intent for a core (`SYSTEM`, `REALTIME`, `BEST_EFFORT`, `ISOLATED`, `SPARE`).
- **Scheduler Class**: The actual algorithm executing on that core (`SYSTEM`, `FIFO_RT`, `DEADLINE_RT`, `FAIR`, `IDLE`).

A single CPU partition role can allow multiple scheduler classes (e.g., a `SYSTEM` role may permit both `SYSTEM` and `FIFO_RT` tasks), and the roles help the runtime validate and orchestrate processes correctly.

## Spatial vs Temporal Partitioning

- **Spatial Partitioning**: On multi-core systems (typically 4+ CPUs), the framework isolates workloads physically by assigning dedicated roles to specific cores (e.g., CPU0 for `SYSTEM`, CPU1/2 for `REALTIME`, CPU3 for `BEST_EFFORT`).
- **Temporal Partitioning**: On small-device systems (1 or 2 CPUs), spatial partitioning is impossible or degraded. The framework falls back to temporal partitioning, where multiple roles and scheduler classes share the same physical core using time slices and priority bands.

## Small-Device Mapping Examples

### 1 CPU
- **Strategy**: Temporal Fallback
- **Mapping**: CPU0 receives the `SYSTEM` role. All enabled classes (e.g., `SYSTEM`, `FIFO_RT`, `FAIR`) are allowed on this single core. Isolation is handled strictly by priority bands, with no dedicated hardware isolation.

### 2 CPU
- **Strategy**: Compressed Mapping
- **Mapping (Mixed Critical)**:
  - CPU0: `SYSTEM` + `REALTIME`
  - CPU1: `BEST_EFFORT` (or `SPARE` if `FAIR` is disabled)

### 4 CPU
- **Strategy**: Spatial Mapping
- **Mapping (Mixed Critical)**:
  - CPU0: `SYSTEM`
  - CPU1: `REALTIME`
  - CPU2: `REALTIME` / `SPARE`
  - CPU3: `BEST_EFFORT`

## Default Configurations

The framework establishes safe, conservative defaults based on the chosen profiles.
- **Automobile (Mixed Critical)**: Favors strict partitioning. Requires at least one `SYSTEM` core and prefers `REALTIME` cores, pushing non-critical tasks to `BEST_EFFORT`.
- **Mobile (Mixed Critical)**: More permissive, allowing dynamic mixed modes.
- **Mobile (General Purpose)**: Defaults to fair scheduling with no dedicated realtime cores.

## Future Work

This framework scaffolds the mechanism for CPU partitioning. Future work will include:
- Fully rewriting the scheduler to inherently utilize these per-core contracts.
- Implementing the Service Supervisor and runtime policy managers.
- Advanced capabilities-based runtime thread reassignment.
- EDF (Earliest Deadline First) scheduler class integration.
