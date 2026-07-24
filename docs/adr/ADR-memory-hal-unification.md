---
title: ADR: Memory HAL Unification and Multikernel Memory Model
status: Accepted
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - adr
see_also:
  - README.md
---
# ADR: Memory HAL Unification and Multikernel Memory Model

## Status
Accepted

## Context
Current system suffers from:
* multiple MMU abstractions
* arch-specific code in HAL
* lack of TLB coordination
* no DMA lifecycle model

Additionally:
* Bharat-OS is **multikernel**
* must support **MMU / MPU / MMU-lite**
* must follow folder structure

## Decision
We will:
1. Introduce unified memory contracts (`hal_pt`, `hal_tlb`, `hal_mpu`, `hal_dma`, `hal_iommu`).
2. Introduce memory backend abstraction (`hal_mem_model_t` and `hal_mem_backend_ops_t`).
3. Enforce per-core memory ownership.
4. Implement message-based TLB coordination.
5. Move arch-specific code to `/core/arch/.../mm`.
6. Remove legacy abstractions (`hal_vmm_*`, `mmu_ops_t`).

## Consequences

### Positive
* scalable multicore architecture
* clean separation of concerns
* supports embedded → server spectrum
* easier hardware portability

### Negative
* higher implementation complexity
* requires strict discipline
* requires test expansion

### Risks
* incorrect TLB coordination → memory corruption
* MPU mismatch → protection gaps
* DMA lifecycle bugs → device issues

### Mitigation
* strong unit tests
* per-arch validation
* capability-based enforcement
