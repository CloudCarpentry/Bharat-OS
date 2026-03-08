# Memory Model

## Overview

Bharat-OS delegates memory management policy entirely to user-space, keeping the kernel minimal. The kernel only provides the mechanisms to map and unmap physical pages to virtual addresses.

## MMU Hardware Abstraction Layer (HAL)

Bharat-OS utilizes a unified `mmu_ops_t` structure (`kernel/include/hal/mmu_ops.h`) to bridge architecture-independent VMM operations and architecture-specific hardware page tables. This decouples the VMM policy from the hardware implementation.

### Layer Architecture

```
┌─────────────────────────────────────────────────┐
│           VMM Core (arch-independent)           │
│   map_page(), unmap_page(), protect(), query()  │
├─────────────────────────────────────────────────┤
│              MMU Abstraction Layer              │
│         struct mmu_ops  (vtable/HAL)            │
├──────────┬──────────┬───────────┬───────────────┤
│  x86_64  │ AArch64  │  RISC-V   │  ARM32 / SOC  │
│  4-level │ 4-level  │  Sv39/48  │  2-level LPAE │
│  PML4    │ TTBRx    │  SATP     │  TTBR0/1      │
└──────────┴──────────┴───────────┴───────────────┘
```

The generic Virtual Memory Manager (`kernel/src/mm/vmm.c`) requests page mappings and modifications using a set of architecture-agnostic flags (`mmu_flags_t` like `MMU_READ`, `MMU_WRITE`, `MMU_USER`). The underlying HAL converts these flags to format-specific mappings at boot.

### Registration and Detection

At early boot, `arch_mmu_init()` identifies the active architecture and assigns a static baseline `mmu_ops_t` profile (e.g. `x86_64_mmu_ops`). It then runs an architecture-specific runtime detection routine (`x86_mmu_detect()`, `arm64_mmu_detect()`, etc.) to elevate capabilities if the hardware supports them (e.g., detecting 1GB huge pages via CPUID, or Sv48 paging on RISC-V).

## The Two Product Lines

Memory allocation wildly diverges depending on the target profile:

### Bharat-RT (Real-Time Embedded)

- **Static Allocation**: All memory for critical tasks is pre-allocated at boot time via capabilities.
- **No Paging**: Demand paging is disabled. Page faults in critical tasks are considered fatal errors, ensuring perfectly bounded latency and deterministic execution.

### Bharat-Cloud (High-Throughput Servers)

- **Demand Paging**: Virtual pages are allocated lazily via page faults resolved by user-space pager daemons.
- **NUMA Readiness**: The kernel interfaces are designed from Day 1 to support per-node descriptors, CPU-to-node affinity, and `memory_node_id` routing. _Note: v1 implementations execute as single-node entities; full NUMA memory-balancing policies are deferred to user-space heuristics in future versions._
- **Distributed Shared Memory (DSM)**: Over CXL 3.x fabrics, the memory model can span across thousands of accelerator nodes seamlessly.
