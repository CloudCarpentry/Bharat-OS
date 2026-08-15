---
title: Bharat-OS HAL Architecture Tiers
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - hal
see_also:
  - README.md
---
# Bharat-OS HAL Architecture Tiers

To ensure build truthfulness and prevent the "illusion of completeness," Bharat-OS classifies architecture support into five distinct Tiers.

## Tier Model

| Tier | Level | Description |
| :--- | :--- | :--- |
| **Tier 0** | `SCAFFOLD_ONLY` | Folder structure and mandatory HAL symbols exist, but no functional code. |
| **Tier 1** | `BUILD_SUPPORTED` | Compiles into object files, but not yet runnable on silicon or simulators. |
| **Tier 2** | `BOOT_SUPPORTED` | Reaches `kernel_main` and early boot console on at least one platform. |
| **Tier 3** | `RUNTIME_SUPPORTED` | Functional scheduling, interrupts, timers, and basic drivers. |
| **Tier 4** | `PRODUCTION_SUPPORTED` | Validated, secure, and production-ready with full test coverage. |

## Current Status

| Architecture | Tier | Notes |
| :--- | :--- | :--- |
| x86_64 | Tier 3 | Active development target (`BHARAT_ARCH_HAS_COMPLETE_USERSPACE`). |
| arm64 | Tier 3 | Active development target (`BHARAT_ARCH_HAS_COMPLETE_USERSPACE`). |
| riscv64 | Tier 3 | Active development target (`BHARAT_ARCH_HAS_COMPLETE_USERSPACE`). |
| arm32 | Tier 2 | Reaches early boot, userspace supported but hardware constraints apply. |
| riscv32 | Tier 2 | Reaches early boot, userspace supported but hardware constraints apply. |
| ARC32 | Tier 1 | Build-supported scaffold (`core/arch/arc/`). |
| TriCore | Tier 1 | Scaffold available (`core/arch/tricore/`). |
| Renesas RX | Tier 1 | Scaffold available (`core/arch/rx/`). |
| SHAKTI | Tier 1 | Scaffold available (`core/arch/shakti/`). |

## Implementation Rules (IMPLEMENTED)

1.  **Fail Closed**: Unsupported HAL operations must return `K_ERR_UNSUPPORTED` (or `BH_ERR_UNSUPPORTED`), never success.
2.  **Honest Reporting**: `hal_get_arch_capabilities()` must report the true Tier and available features.
3.  **Separation**:
    -   `core/arch/<isa>/`: ISA-specific mechanics (registers, context switch).
    -   `core/hal/`: Architecture-neutral contracts and common helpers (`core/hal/include/hal/`, `core/hal/mmu/`, `core/hal/mmu_lite/`, `core/hal/prot/`).
    -   `core/platform/`: Board and SoC-specific wiring.
