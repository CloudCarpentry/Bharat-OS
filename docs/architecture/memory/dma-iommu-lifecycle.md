# DMA and IOMMU Lifecycle Contract

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: ✅ Core kernel lifecycle guardrails merged (`mm/dma`)
- **Validated**: 🚧 Host lifecycle tests added; full stress/fault-injection remains pending


## Overview
This document defines the strict lifecycle and ownership model for device memory access, ensuring safe, coherent, and isolated data transfers between hardware accelerators and system memory. This contract spans the `hal_dma`, `hal_iommu`, and kernel `mm/iommu` layers.

## Core Principles
- **Explicit Ownership:** Memory mapped for DMA is strictly owned by the device until unmapped or synchronized back to the CPU.
- **Coherency Awareness:** The system explicitly distinguishes between hardware-coherent devices and non-coherent devices. Cache maintenance operations are mandatory for non-coherent transfers.
- **IOMMU Isolation:** If an IOMMU is present (`HW_CAP_STATE_PRESENT` or higher in `bharat_hw_caps_t`), all DMA-capable devices must be bound to a protection domain. Direct physical bypass is forbidden unless explicitly permitted by policy.

## DMA Lifecycle

### 1. Allocation
Allocations intended for DMA should be requested via the allocator with `MEM_DMA` as the `alloc_class_t`.

### 2. Mapping (`hal_dma_map`)
- CPU prepares the buffer.
- `hal_dma_map` is called to generate a device-visible address (IOVA or physical).
- Ownership transitions: **CPU -> Device**.
- For non-coherent architectures, this step implies a **cache clean** (flush to RAM).
- Kernel enforcement: mapping is rejected unless the buffer is pinned and currently CPU-owned.
- Kernel enforcement: repeated map attempts without unmap are rejected.

### 3. Execution
- Device performs the read/write operations using the mapped address.
- CPU *must not* access the memory during this phase.

### 4. Synchronization (`hal_dma_sync`)
- Required for streaming/reused buffers.
- `hal_dma_sync(SYNC_FOR_CPU)`: Ownership transitions **Device -> CPU**. Implies **cache invalidate**.
- `hal_dma_sync(SYNC_FOR_DEVICE)`: Ownership transitions **CPU -> Device**. Implies **cache clean**.
- Kernel enforcement: sync operations are direction-validated and state-aware so invalid ownership transitions are ignored.

### 5. Unmapping (`hal_dma_unmap`)
- Device is finished with the buffer.
- `hal_dma_unmap` revokes device access.
- Ownership transitions: **Device -> CPU**.
- For non-coherent architectures, this implies a final **cache invalidate** (if device wrote data).
- Kernel enforcement: unmap direction must match the active mapping direction, and unmap on an unmapped buffer is rejected.

## IOMMU Insertion Points

If the system supports an IOMMU, it intercepts the `hal_dma_map` calls.
- `hal_iommu_attach_device`: Binds a hardware device to a specific IOMMU protection domain.
- `hal_iommu_map`: Creates the IOVA -> Physical mapping in the IOMMU page tables.
- `hal_iommu_unmap`: Tears down the IOVA mapping.
- Kernel enforcement: if IOMMU map fails, DMA map is aborted and no device-ownership transition occurs.

## Current Implementation Notes (April 22, 2026)
- Implemented in `kernel/src/mm/dma/dma.c`:
  - explicit map/unmap state tracking (`mapped_to_device`, `owned_by_device`, `active_dir`)
  - pin-before-map validation
  - HAL DMA map/unmap integration plus non-coherent sync hooks
  - IOMMU map error propagation and rollback semantics
- Host validation in `tests/test_dma_lifecycle_host.c` covers:
  - map/unmap ownership transitions
  - double-map rejection
  - sync invocation on non-coherent path
  - IOMMU mapping failure handling

## Coherent vs. Non-Coherent Path

| Architecture capability | DMA Mapping behavior | DMA Sync behavior |
| :--- | :--- | :--- |
| **Fully Coherent** (e.g., standard x86_64, some ARM64 servers) | No cache operations needed. Returns IOVA/Phys immediately. | No-op. Hardware snoops caches. |
| **Non-Coherent** (e.g., most ARM/RISC-V embedded, edge) | Cache Clean (flush to RAM) on mapping. | Cache Invalidate (CPU) or Clean (Device) based on sync direction. |
