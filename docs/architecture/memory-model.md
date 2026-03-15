# Memory Model

## Overview

Bharat-OS employs a **3-plane Distributed Virtual Memory Architecture** designed to span multiple nodes, hardware profiles (MMU/MPU), and real-time constraints, while delegating policy and resolution to user-space or URPC-based monitors where appropriate.

## The 3-Plane Architecture

See [ADR 008](../adr/008-distributed-vm-monitor-and-vm-spaces.md) for full context on this transition.

### Plane A: Canonical Distributed VM Model
The **source of truth** for virtual memory. Instead of one monolithic page-table object, the OS uses `vm_space_t` to track the canonical state of address spaces, mapping rules, generations, and profile compatibility. It knows if a mapping is meant for an MPU system, an MMU system, or an IOMMU isolated domain. It manages "Timing Classes" directly (`Best-Effort`, `Soft RT`, `Firm RT`, `Hard RT`).

### Plane B: URPC Monitor Protocol
The **distributed control plane**. Changes to the canonical `vm_space_t` (e.g., mapping adds, revokes, unmaps) increment a generation counter. The "Home Monitor" for that address space uses URPC messages (`MON_VM_MAP`, `MON_VM_UNMAP`, `MON_VM_TLB_INVALIDATE_RANGE`) to broadcast the change. Other cores ("Remote Monitors") receive these messages, update their local view, and return an ACK (in strict sync mode) or lazily realize them later (in lazy mode).

### Plane C: Local Hardware Realization
The **silicon-facing data plane**. Hardware page tables (or MPU regions) are strictly treated as **local, cached realizations** of the canonical state. The `arch_vm_ops_t` and `arch_mem_domain_ops_t` structures decouple the core VMM policy from architecture-specific mechanisms (e.g., writing x86_64 PML4 entries, configuring ARM64 TTBR0/1, or RISC-V Sv39 `satp` registers).

```
┌────────────────────────────────────────────────────────┐
│ Plane A: Canonical VM (vm_space_t) - Source of Truth   │
│   Manages capabilities, mappings, and timing classes   │
├────────────────────────────────────────────────────────┤
│ Plane B: URPC Monitor Protocol - Coordination          │
│   MON_VM_MAP, MON_VM_UNMAP, Generation Sync, ACKs      │
├────────────────────────────────────────────────────────┤
│ Plane C: Hardware Realization (arch_vm_ops_t)          │
├──────────┬──────────┬───────────┬───────────┬──────────┤
│  x86_64  │ AArch64  │  RISC-V   │ ARM MPU   │ IOMMU    │
│  PML4    │ TTBRx    │  SATP     │ Regions   │ Domains  │
└──────────┴──────────┴───────────┴───────────┴──────────┘
```

## Profiles and Timing Classes

Memory allocation wildly diverges depending on the target profile, explicitly baked into the `vm_space_t`:

### Hard Real-Time (Bharat-RT)
- **Profile:** MPU/PMP (`MEM_PROFILE_MPU_ONLY`) or Hard RT MMU (`MEM_PROFILE_MMU_BASIC`).
- **Constraints:** Fully resident, pinned, and prefaulted mappings. No demand paging or runtime page-table allocation. Page faults in critical tasks are fatal. A core must achieve the `VM_REALIZE_RT_VALID` state before the scheduler dispatches a task.

### Cloud/Edge (Bharat-Cloud)
- **Profile:** Distributed MMU (`MEM_PROFILE_MMU_DISTRIBUTED`) or DMA Isolated (`MEM_PROFILE_MMU_DMA_ISOLATED`).
- **Constraints:** Demand paging, lazy realization of mappings, thread migration with remote fault recovery. Unmaps use strict generation-sync revocation to prevent use-after-free vulnerabilities. Full IOMMU/SMMU domain support.
- **NUMA Readiness**: The kernel interfaces are designed from Day 1 to support per-node descriptors, CPU-to-node affinity, and `memory_node_id` routing.
