---
title: KMEM-ACCEL-001 - Hardware-Assisted Secure Memops
status: Draft
owner: Architecture Working Group
last_updated: 2026-08-09
tags:
  - docs
  - architecture
  - kernel
  - memory
see_also:
  - README.md
---

# KMEM-ACCEL-001: Hardware-Assisted Secure Zero/Copy/Cache Operations

## Context
PMM and slab allocators can benefit from hardware acceleration for zeroing and cache maintenance without exposing ISA details.

## Design
Add neutral operations dispatched through the capability layer to optimized hardware implementations.

### Neutral API

```c
void bh_mem_zero_page(void *addr);
void bh_mem_zero_secure(void *addr, size_t len);
void bh_cache_clean_range(void *addr, size_t len);
void bh_cache_invalidate_range(void *addr, size_t len);
void bh_cache_flush_range(void *addr, size_t len);
```

### ISA Mapping
* **RISC-V**: CMO extensions (Zicboz for zeroing, block clean/flush).
* **Arm**: Architectural cache zero, cache maintenance operations.
* **x86**: Optimized string memops (e.g., ERMS).

## Architecture Dispatch

```mermaid
flowchart TD
    A[Neutral Memory API Call] --> B{Hardware Implementation Available?}
    B -->|Yes| C[Hardware Implementation]
    B -->|No| D{Optimized Arch Implementation?}
    D -->|Yes| E[Optimized Architecture Implementation]
    D -->|No| F[Freestanding Scalar Fallback]
```

### Tier-0 fallback contract

`core/hal/common/memops/mem_scalar.c` is the single architecture-neutral
Tier-0 authority. It uses only requested byte loads and stores: no prefetch,
word-sized access, SIMD/vector state, DMA, cache/topology assumptions, or calls
to another memory primitive. Architecture directories own the dispatched
`hal_memcpy()`, `hal_memset()`, and `hal_memmove()` entry points.

IRQ-safe and early-boot dispatch must select Tier 0. RV32 remains scalar-only
until an XLEN-neutral GPR implementation is independently qualified; RV64
memops objects are not valid RV32 providers. Tier 0 `memmove` determines copy
direction using overflow-safe `uintptr_t` address differences and never forms
an unchecked end pointer.

## Execution Plan
1. **Define Neutral API**: Create the standard functions for zeroing and cache maintenance.
2. **Capability Hooks**: Map `hal_hw_caps_t` flags (like `cache_block_zero`) to the dispatch logic.
3. **Hardware Implementations**: Implement assembly/intrinsic routines for supported ISAs (Zicboz, ERMS, etc.).
4. **Fallback Routines**: Ensure freestanding scalar implementations exist for unsupported targets.
5. **Security Check**: Enforce that the kernel mediates all raw cache maintenance operations; do not expose directly to userspace unless explicitly authorized.
6. **Testing**: Verify cache coherency after operations and measure zeroing performance improvements.
