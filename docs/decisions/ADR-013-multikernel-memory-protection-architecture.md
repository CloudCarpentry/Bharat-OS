# ADR-013: Multikernel Memory Protection Architecture (MPA)

## Status
Proposed

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

## Next Steps

Bring up and validate the single-core page-table HAL on `x86_64` first, then generalize the same contract to `arm64` and `riscv64`. Introduce URPC-based shootdown only once SMP makes cross-core TLB invalidation necessary.

1. Define the capability word and feature bits for VIRT, ASID, HUGEPAGE, NX, IOMMU, and region-only MPU mode.
2. Define the `mem_protect_ops_t` split into `cpu_ops` and `iommu_ops`.
3. Add the `x86_64` `cpu_ops` backend.
4. Wire `vmm_map()` / `vmm_unmap()` to update hardware mappings synchronously via the HAL.
5. Add the `owner_core` property to frame capability metadata immediately.
6. Add the `arm64` and `riscv64` backends using the same vtable contract.
7. Provide a runtime IOMMU probe hook (which may return NULL).
8. Implement the uRPC shootdown and ACK protocols (deferred until SMP relevance exists; local TLB flush is sufficient for single-core bring-up).
