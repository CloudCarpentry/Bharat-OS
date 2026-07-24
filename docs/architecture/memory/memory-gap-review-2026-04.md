# Memory Architecture Review and Gap Analysis (April 2026)

## 1. Overview
This document reviews the state of the Bharat-OS memory architecture as of April 2026, specifically focusing on the transition to a production-grade multikernel memory model.

## 2. Recent Improvements (April 2026 PR Slice)
The following foundational components have been implemented to harden the memory architecture:

- **HAL Memory Capability Contract**: Established a canonical API (`hal_mem_get_caps`) for reporting hardware-level memory features (MMU model, ASID, NX, Hugepages, IOMMU presence).
- **Runtime Model Validation**: Introduced a boot-time validator (`mm_validate_model`) that reconciles hardware reality with requested software profiles, ensuring the OS fails closed on insecure or unsupported configurations.
- **Policy Guards**: Integrated runtime-validated model checks into the VMM and memory class allocation paths to reject illegal operations (e.g., arbitrary page mapping on MPU-only targets).
- **Architecture Coverage**: Implemented capability reporting for `x86_64`, `arm64`, `riscv64`, `arm32`, and `riscv32`.

## 3. Current Gaps and Future Phases

### Phase 1: Bounded TLB Shootdown Protocol (High Priority)
- **Gap**: Cross-core TLB invalidation currently relies on a simple uRPC message without strict completion guarantees or bounded wait times.
- **Goal**: Implement a robust shootdown state machine with sequence numbers, ACKs, and timeout handling.

### Phase 2: Unified Virtual Memory Authority Path
- **Gap**: Some legacy paths still invoke HAL/Arch mapping functions directly, bypassing the `address_space_t` authority.
- **Goal**: Deprecate direct HAL/Arch mapping calls and enforce the `VMM -> ASpace -> Region -> HAL` pipeline strictly.

### Phase 3: DMA and IOMMU Lifecycle Hardening
- **Gap**: IOMMU support is currently stubbed or partially implemented in some profiles. DMA mapping lacks a unified lifecycle state machine across all architectures.
- **Goal**: Finalize the `hal_iommu` and `hal_dma` integration with the kernel's DMA grant capability model.

### Phase 4: PMM Ownership and Fragmentation
- **Gap**: Per-core PMM caches (magazines) are implemented but lack sophisticated cross-core rebalancing for large contiguous allocations.
- **Goal**: Implement a distributed physical memory rebalancing algorithm for multikernel environments.

## 4. Conclusion
The memory architecture is moving toward a highly deterministic, capability-gated model. The recent introduction of runtime validation ensures that security properties are enforced from the earliest boot stages.
