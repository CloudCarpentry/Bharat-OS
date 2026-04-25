---
title: ADR-013: Multikernel Memory Protection Architecture (MPA)
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - adr
see_also:
  - README.md
---
# ADR-013: Multikernel Memory Protection Architecture (MPA)

## Status
Accepted (Actively Implemented)

## Context
Bharat-OS operates on a multikernel architecture, avoiding global shared state where possible. Historically, monolithic kernels (like Linux) utilize a single shared page table that all CPUs look up under a lock. This approach limits scalability and creates contention during heavy memory mapping and unmapping operations.

Furthermore, memory protection across varying domains—such as CPUs and devices (via DMA)—often results in hardware-specific logic leaking into the higher-level kernel abstractions. Treating these varying memory access requirements via compile-time "pick one" logic forces inflexible builds and tight coupling.

## Decision
We will treat CPU memory protection (MMU/MMU-lite/MPU) and device DMA protection (IOMMU) as two independent axes. These will be unified by one capability word and one Memory Protection Architecture (MPA) abstraction, with runtime composition instead of compile-time logic. The kernel will read capability bits (such as VIRT, ASID, HUGEPAGE, NX, IOMMU, and region-only MPU mode) through a single unified `mem_protect_ops_t` API that splits into `cpu_ops` and `iommu_ops`.

This leads to a three-layer implementation format:
1. **HAL Page-Table VTable**: Each architecture provides five focused functions for allocating, mapping, unmapping, setting the root, and locally flushing the TLB.
2. **VMM Mapping Registry**: The software mirror of the hardware page table. `vmm_map()` immediately calls the HAL map page function. Each entry records the capability token owning the backing frame.
3. **uRPC Shootdown Protocol**: To enforce correct behavior without cross-core locks, local TLB invalidation on remote cores will occur via asynchronous `SHOOTDOWN` uRPC messages, and the unmapping core will wait for an ACK.

Additionally, this approach relies on a **frame ownership invariant**: every physical frame has exactly one owner core at any moment, tracked within capability table metadata (`owner_core`), completely eliminating the class of cross-core page-table race conditions.

## Consequences
**Positive**:
- Eliminates cross-core locking on the page-table walker itself.
- High scalability; each core runs its own kernel instance with its own local page-table root.
- Cleanly separates device memory protection from CPU memory protection.
- Eliminates hardware-specific compile-time logic in the kernel VMM.

**Negative**:
- Adds slightly more complex messaging logic via uRPC for shootdowns.
- Requires modifying the capability model to embed ownership information and handle remote capability modifications.

## Implementation Status (Update)

The memory protection architecture is now actively in-tree, driven by capability-gated feature flags (`BHARAT_ENABLE_MMU`, `BHARAT_ENABLE_MPU`, `BHARAT_ENABLE_ADVANCED_VM`, etc.).

1. **DONE:** The capability word and feature bits for varying capabilities are defined and passed contextually.
2. **DONE:** HAL Translation interfaces (`hal_pt`, `hal_translate_ops`) exist, completely replacing the legacy global MMU state.
3. **DONE:** Hardware implementations for `x86_64`, `arm64`, `riscv64`, as well as 32-bit MPU/MMU-lite environments (`arm32`, `riscv32`) exist and are integrated.
4. **DONE:** The capability token and frame ownership models ensure zero cross-core locking inside the physical page allocator (PMM) and logical address spaces.
5. **PARTIAL:** URPC shootdown pathways are functionally scaffolded via asynchronous messages, undergoing hardening for larger-scale SMP.
6. **PARTIAL:** Device IOMMU/DMA protection is functionally isolated via memory domains, but full hardware programming (e.g. SMMU, VT-d) remains to be completed.
