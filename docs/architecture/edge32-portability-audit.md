---
title: EDGE32 Portability Audit
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
see_also:
  - README.md
---
# EDGE32 Portability Audit

This document summarizes the current status of pointer-width inference and portability assumptions that must be addressed before the full arm32 and riscv32 (EDGE32 Tier 2) ports can be considered fully functional.

## Address and Size Types (`uintptr_t`, `size_t`, `vaddr_t`, `paddr_t`)
- **Status**: Risky
- **Details**: The codebase currently assumes `sizeof(void *) == 8` implicitly in many places. Physical memory management, page tables, and various drivers assume a 64-bit wide `paddr_t` and `vaddr_t`. Some fast-path logic (like `memops_scalar.c`) relies on `UINTPTR_MAX > 0xFFFFFFFF` to determine if a fast path optimization should compile.
- **Action**: Must fix before RV32 and ARM32 bring-ups.

## Page-Table Entry Abstractions & PFN Width Assumptions
- **Status**: Risky
- **Details**: HAL page table definitions and Physical Memory Management (`pmm.c`) contain `#if defined(__x86_64__)` or `__aarch64__` blocks that dictate physical layout and page table structures. The PFN width logic currently defaults to 64-bit bounds in some structures.
- **Action**: Create proper generic MMU and PTE interfaces that are correctly configured by the `ARCH_CAP_MMU_FULL` and related capabilities rather than purely CPU architecture checks. Must fix before arm32.

## Syscall Argument Packing
- **Status**: Needs Audit
- **Details**: Kernel entry points and syscall abstractions must cleanly serialize arguments regardless of whether registers are 32 or 64-bit wide. This likely requires specialized ABI definitions in `core/kernel/include/core/arch/` to marshal 64-bit values into two 32-bit registers for Tier 2 architectures.
- **Action**: Must fix before RV32/ARM32 bring-ups.

## Capability/Object Identifiers
- **Status**: OK
- **Details**: Existing types seem somewhat decoupled from pointer widths natively. Further testing required.

## Timeout/Tick Math
- **Status**: OK (needs testing)
- **Details**: 64-bit integer timer bounds are largely safe when using standard `uint64_t` types provided the target CPU provides instructions to read high/low 32-bit registers atomically. RV32 and ARM32 both support 64-bit timers through split reads.

## Bitmap/Cpumask Widths
- **Status**: Risky
- **Details**: Multi-core support (`smp_init`, etc.) makes assumptions about SMP being enabled by default. `arch_has_cap(ARCH_CAP_SMP)` should replace these inferences to allow UP configurations in edge tier without wasting space or assuming large CPUMASK structures.

## Cache/DMA Assumptions
- **Status**: Needs Refactor
- **Details**: DMA coherent checks and caching instructions are highly specific to ARM64 and X86_64. Tier 2 devices might lack hardware-coherent DMA, meaning cache maintenance must be explicitly handled. Replace hardware DMA assumptions with checks for `ARCH_CAP_DMA_COHERENT`.
- **Action**: Must fix before arm32 bring-up.
