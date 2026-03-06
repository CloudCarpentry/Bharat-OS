# Memory Model

## Overview

Bharat-OS delegates memory management policy entirely to user-space, keeping the kernel minimal. The kernel only provides the mechanisms to map and unmap physical pages to virtual addresses.

## The Two Product Lines

Memory allocation wildly diverges depending on the target profile:

### Bharat-RT (Real-Time Embedded)

- **Static Allocation**: All memory for critical tasks is pre-allocated at boot time via capabilities.
- **No Paging**: Demand paging is disabled. Page faults in critical tasks are considered fatal errors, ensuring perfectly bounded latency and deterministic execution.

### Bharat-Cloud (High-Throughput Servers)

- **Demand Paging**: Virtual pages are allocated lazily via page faults resolved by user-space pager daemons.
- **NUMA Readiness**: The kernel interfaces are designed from Day 1 to support per-node descriptors, CPU-to-node affinity, and `memory_node_id` routing. _Note: v1 implementations execute as single-node entities; full NUMA memory-balancing policies are deferred to user-space heuristics in future versions._
- **Distributed Shared Memory (DSM)**: Over CXL 3.x fabrics, the memory model can span across thousands of accelerator nodes seamlessly.
