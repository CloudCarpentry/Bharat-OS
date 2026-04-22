# DMA and IOMMU Lifecycle Contract

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: 🚧 Pending kernel/service behavior merge
- **Validated**: ❌ Pending stress/fault-injection tests


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

### 3. Execution
- Device performs the read/write operations using the mapped address.
- CPU *must not* access the memory during this phase.

### 4. Synchronization (`hal_dma_sync`)
- Required for streaming/reused buffers.
- `hal_dma_sync(SYNC_FOR_CPU)`: Ownership transitions **Device -> CPU**. Implies **cache invalidate**.
- `hal_dma_sync(SYNC_FOR_DEVICE)`: Ownership transitions **CPU -> Device**. Implies **cache clean**.

### 5. Unmapping (`hal_dma_unmap`)
- Device is finished with the buffer.
- `hal_dma_unmap` revokes device access.
- Ownership transitions: **Device -> CPU**.
- For non-coherent architectures, this implies a final **cache invalidate** (if device wrote data).

## IOMMU Insertion Points

If the system supports an IOMMU, it intercepts the `hal_dma_map` calls.
- `hal_iommu_attach_device`: Binds a hardware device to a specific IOMMU protection domain.
- `hal_iommu_map`: Creates the IOVA -> Physical mapping in the IOMMU page tables.
- `hal_iommu_unmap`: Tears down the IOVA mapping.

## Coherent vs. Non-Coherent Path

| Architecture capability | DMA Mapping behavior | DMA Sync behavior |
| :--- | :--- | :--- |
| **Fully Coherent** (e.g., standard x86_64, some ARM64 servers) | No cache operations needed. Returns IOVA/Phys immediately. | No-op. Hardware snoops caches. |
| **Non-Coherent** (e.g., most ARM/RISC-V embedded, edge) | Cache Clean (flush to RAM) on mapping. | Cache Invalidate (CPU) or Clean (Device) based on sync direction. |
