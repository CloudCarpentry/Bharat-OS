---
title: ARM32 + RV32 EDGE-Tier Architecture Expansion Plan
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
see_also:
  - README.md
---
# ARM32 + RV32 EDGE-Tier Architecture Expansion Plan

This document defines how Bharat-OS should add 32-bit architecture support without splitting into separate kernel personalities.

## Decision Summary

Bharat-OS will support five CPU families in two runtime tiers:

- **Tier 1 (Full Baseline):** `x86_64`, `arm64`, `riscv64`
- **Tier 2 (EDGE32 Compact Ports):** `arm32`, `riscv32`

Tier 2 is first-class in product scope, but starts with a smaller mandatory feature envelope to avoid premature parity churn.

## Non-Negotiable Rule

Do not fork kernel semantics into a “64-bit OS” and a separate “32-bit special-case OS.”

Core kernel contracts must remain:

- XLEN-neutral
- page-table-model neutral
- interrupt-controller neutral
- cache/DMA coherency-model neutral
- capability format versioned where needed

Architecture backends may vary in feature richness, but not in core security and IPC semantics.

## Architecture Capability Matrix

Common code should key off declared architecture capabilities, not inferred pointer width assumptions.

### Proposed capability flags

- `ARCH_CAP_64BIT_VA`
- `ARCH_CAP_SMP`
- `ARCH_CAP_MMU_FULL`
- `ARCH_CAP_DMA_COHERENT`
- `ARCH_CAP_ADV_IRQ_ROUTING`
- `ARCH_CAP_USERSPACE_HIGHHALF`
- `ARCH_CAP_HW_CRC`
- `ARCH_CAP_SIMD_NET_CSUM`

### Tier baseline expectations

| Capability / Feature | Tier 1 (x86_64, arm64, riscv64) | Tier 2 (arm32, riscv32) |
| --- | --- | --- |
| Core capability syscall semantics | Required | Required |
| Endpoint IPC contract | Required | Required |
| Scheduler/fault model | Required | Required |
| SMP | Required | Optional initially (UP-first allowed) |
| Advanced IRQ affinity/routing | Preferred/required by platform | Optional initially |
| VM/aspace depth | Full model | Reduced model allowed |
| DMA/cache maintenance richness | Full model | Reduced model allowed |
| Userspace VA range | Large | Reduced bounds allowed |

## Implementation Guidance

### 1) Portability cleanup before new bring-up

Audit and normalize shared core/kernel/HAL code for 32/64-bit safety:

- address and size types (`uintptr_t`, `size_t`, `vaddr_t`, `paddr_t`)
- page-table entry abstractions
- PFN width assumptions
- syscall argument packing
- capability/object identifiers
- timeout/tick math
- bitmap and cpumask widths
- cache/DMA assumptions

### 2) Keep contracts stable, vary richness by capability

Must remain architecture-invariant:

- capability semantics
- syscall ABI contract
- endpoint/IPC semantics
- scheduler model
- fault contract
- driver/service isolation model

Allowed to vary by tier:

- page-table depth
- ASID space richness
- TLB invalidation sophistication
- MSI/MSI-X style routing support
- SIMD acceleration
- cache maintenance detail
- userspace VA size

### 3) Keep 32-bit HAL backends explicit

Avoid spreading `#ifdef` logic across 64-bit backends.

Use dedicated backend trees:

- `core/kernel/src/core/hal/arm32/...`
- `core/kernel/src/core/hal/riscv32/...`

Each backend should own its own:

- trap/exception entry
- MMU/PT implementation
- timer + interrupt controller integration
- cache/DMA operations
- boot path and board glue

### 4) Start with narrow reference platforms

Do not attempt broad architecture claims at initial merge.

- **ARM32 first target:** one MMU-capable Cortex-A class platform
- **RV32 first target:** one deterministic QEMU/SoC-style platform with known timer + interrupt + memory map

### 5) Recommended phase plan

1. **Phase 1 – Portability cleanup:** make common code XLEN-safe.
2. **Phase 2 – Capability matrix:** codify required vs optional arch features.
3. **Phase 3 – ARM32 bring-up:** minimal boot + trap + timer + IRQ + MMU + context switch.
4. **Phase 4 – RV32 bring-up:** same minimum on one reference platform.
5. **Phase 5 – EDGE tuning:** memory footprint, lean service set, and profile-level optimization.

## Repository Alignment Notes

The repository already documents full HAL paths for `x86_64`, `arm64`, and `riscv64`, and identifies EDGE targets (wearables, robotics, drones) as explicit product profiles. This plan extends that trajectory while containing complexity growth.

The plan also aligns with the existing statement that runtime maturity differs by architecture target, by making maturity and capability deltas explicit and testable.
