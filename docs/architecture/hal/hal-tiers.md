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
| x86_64 | Tier 3 | Active development target. |
| arm64 | Tier 3 | Active development target. |
| riscv64 | Tier 3 | Active development target. |
| arm32 | Tier 2 | Reaches early boot. |
| riscv32 | Tier 2 | Reaches early boot. |
| ARC32 | Tier 1 | Build-supported scaffold. |
| TriCore | Tier 0 | Roadmap scaffold. |
| Renesas RX | Tier 0 | Roadmap scaffold. |

## Implementation Rules

1.  **Fail Closed**: Unsupported HAL operations must return `K_ERR_UNSUPPORTED` (or `BH_ERR_UNSUPPORTED`), never success.
2.  **Honest Reporting**: `hal_get_arch_capabilities()` must report the true Tier and available features.
3.  **Separation**:
    -   `core/arch/<isa>/`: ISA-specific mechanics (registers, context switch).
    -   `core/arch/hal/<isa>/`: Architecture-specific implementation of HAL contracts.
    -   `core/hal/`: Architecture-neutral contracts and common helpers.
    -   `core/platform/`: Board and SoC-specific wiring.
