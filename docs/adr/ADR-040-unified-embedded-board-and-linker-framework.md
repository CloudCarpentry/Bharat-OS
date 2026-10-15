---
title: ADR-040 Unified Embedded Board Support and Memory-Aware Linker Framework
status: Accepted
owner: Architecture Working Group & Platform Team
last_updated: 2026-10-15
tags:
  - adr
  - architecture
  - bsp
  - linker
  - mpu
  - arm32
  - riscv32
  - renode
see_also:
  - ADR-013-multikernel-memory-protection-architecture.md
  - ADR-021-orthogonal-userspace-runtime-model.md
  - ADR-039-hybrid-shared-memory-multikernel-concurrency.md
  - ../architecture/embedded-board-support-framework.md
---

# ADR-040: Unified Embedded Board Support and Memory-Aware Linker Framework

## Context

Bharat-OS targets both high-end 64-bit systems (`x86_64`, `arm64`, `riscv64`) with full virtual memory (`MMU_FULL`) and resource-constrained 32-bit embedded microcontrollers and IoT processors (`arm32`, `riscv32`) operating under `MMU_LITE` or `MPU_ONLY`.

Prior to this decision, 32-bit targets relied on hardcoded monolithic linker scripts (such as `core/kernel/linker_arm32.ld` and `core/kernel/linker_riscv32.ld`). This created critical hardware-level failures during simulation and hardware bring-up:
1. **MMIO Address Space Collisions**: The hardcoded ARM32 linker placed kernel text and data at `0x40010000`. On ARM Cortex-M microcontrollers such as the STM32F407, `0x40010000` lies directly inside the APB2 peripheral MMIO register range (`TIM1`), causing the CPU to execute garbage and enter unrecoverable HardFault loops on boot in Renode simulation.
2. **Privilege & Boot Protocol Mismatches**: The default RV32 linker assumed execution starting at `0x80200000` as an OpenSBI payload in Supervisor mode (S-mode). However, resource-constrained RV32 microcontrollers (e.g., SiFive FE310 / HiFive1 Rev B) run in Machine mode (M-mode) directly out of SPI flash (`0x20000000`), where OpenSBI is absent and Physical Memory Protection (PMP) registers are strictly M-mode accessible.
3. **Monolithic Linker Duplication**: Adding or qualifying a new SoC or board required manual duplication of linker scripts, leading to configuration drift, unchecked stack overflow risks, and lack of pre-boot memory safety verification.

## Decision

We establish the **Unified 32-Bit Embedded Board Support and Linker Framework** (`BH-EMBED-LINK-P0-001`). The architecture is structured as follows:

```
platform/boards/<board_id>/board.yaml          (Declarative Memory & Device Manifest)
                  │
                  ▼
tools/board/linker_generator.py ──────────────► delivery/linker/generated/<board>.ld
                  ▲
                  │
delivery/linker/templates/<hardware_class>.ld.in (Templated Linker Authority)
                  │
                  ▼
tools/board/validate.py ──────────────────────► Automated 10-Check Qualification Gate
```

### 1. Four Hardware Classes
The framework categorizes all 32-bit embedded targets into four explicit hardware classes:

| Hardware Class | Privilege Mode | Memory Protection | Vector Alignment | Example Platforms |
|---|---|---|---|---|
| **`cortex_m`** (`armv7e-m`) | Thread/Handler (Priv/Unpriv) | PMSAv7 / PMSAv8 MPU | 4-byte (word-aligned) | STM32F407 Discovery, MPS2-AN386 |
| **`riscv32_mcu`** (`rv32-mcu`) | Machine / User (M/U-mode) | RISC-V PMP (NAPOT/TOR) | 4-byte (word-aligned) | SiFive HiFive1 Rev B (FE310), Shakti |
| **`arm32_mmu_lite`** (`armv7-a`) | PL1 SVC / PL0 User | Static/Eager VMSAv7 Page Tables | 32-byte (`VBAR`) | QEMU Virt Cortex-A15, i.MX6 |
| **`riscv32_sv32`** (`rv32-sv32`) | Supervisor / User (S/U-mode) | Sv32 Paging (2-level) | 4-byte (`stvec`) | QEMU Virt RV32, PolarFire SoC |

### 2. Declarative Board Manifests (`platform/boards/<board_id>/board.yaml`)
Every board specifies its exact memory topology validated against `tools/schemas/board.schema.yaml`:
- **Execution Class**: `cortex_m`, `riscv32_mcu`, `arm32_mmu_lite`, or `riscv32_sv32`.
- **Memory Regions**: Origin, length, access attributes (`rx`, `rw`, `rwx`), and memory type (`flash`, `sram`, `dtim`, `ram`).
- **Stack Budgets**: Bounded kernel, IRQ, and fault stacks.
- **MMIO Exclusion Zones**: Start address, end address, and description of peripheral registers (e.g., APB, AHB, CLINT, PLIC).
- **MPU / PMP Protection Plan**: Fixed region assignments and alignment constraints.
- **Hardware Peripherals**: Console UART, timer, interrupt controller, and simulator mapping (`repl` file).

### 3. Templated Linker Scripts (`delivery/linker/templates/`)
Linker generation is decoupled from board-specific geometry using parameterized templates (`*.ld.in`). Templates guarantee:
- Placement of vector tables (`.isr_vector`, `.vector_table`) at executable flash/ROM origin.
- Placement of read-only sections (`.text`, `.rodata`) in ROM/Flash.
- Initialized data (`.data`) loaded from Flash to SRAM at reset (`AT(ADDR(.rodata) + SIZEOF(.rodata))`).
- Deterministic BSS (`.bss`) zeroing boundaries.
- **Compile-Time Stack Bounds Assertion**:
  `ASSERT(_sram_end >= _stack_top, "Error: Insufficient SRAM for stack allocation!");`

### 4. Link-Time MMIO Collision Detection & Qualification (`tools/board/validate.py`)
Before an image can be flashed or simulated, `tools/board/validate.py` executes a mandatory 10-check qualification suite:
1. **CPU & Compiler ABI Compatibility**: Verifies CPU architecture, FPU options, and ABI compatibility (`soft` vs `hard`).
2. **Flash Layout Validation**: Confirms boot vector and code originate within real non-volatile memory bounds.
3. **SRAM & Reserved Stack Fit**: Verifies that `.data`, `.bss`, and stacks fit within available SRAM without overflow.
4. **Vector Table Boundary & Alignment**: Confirms vector table alignment matches CPU architectural requirements.
5. **MPU/PMP Plan Representability**: Validates power-of-two (PMSAv7) or NAPOT/TOR (PMP) alignment.
6. **No MMIO Overlap**: **Mathematical bounding box intersection check** guaranteeing that no RAM or Flash region intersects any peripheral MMIO address space.
7. **Required UART/Timer Devices**: Checks presence of operational serial console, monotonic timer, and interrupt controller.
8. **Simulator Configuration**: Verifies existence and syntax of Renode `.repl` and `.resc` scripts.
9. **ELF Binary Inspection**: Inspects ELF32 headers (`EM_ARM`, `EM_RISCV`, entry point address, program headers).
10. **Image Execution Readiness**: Final aggregate certification gate.

## Consequences

### Positive
- **Zero Kernel Bloat**: The core microkernel (`core/kernel/`) remains completely free of board-specific `#ifdef`s or hardcoded peripheral base addresses.
- **Prevent HardFaults & MMIO Clashes**: The STM32F4 APB2 MMIO clash (`0x40010000`) is eliminated; kernel code resides at Flash `0x08000000` and RAM at `0x20000000`.
- **Pre-Boot Verification**: Qualification runs in less than 2 seconds via `python tools/board/validate.py --all` or `nirmaan verify --all-boards`.
- **Extensible Board Onboarding**: Adding support for a new microcontroller requires creating a declarative `board.yaml` without writing custom linker scripts.

### Negative / Trade-offs
- Build systems must execute linker generation (`tools/board/linker_generator.py`) prior to compiling embedded targets when custom memory maps are used.
- Renode emulation currently verifies static memory layout and execution pipeline; bare-metal Thumb-2 exception handlers (`BH-EMBED-BOOT-P0-002`) remain required for full hardware reset.

## References
- [Board Schema](file:///platform/boards/stm32f407_discovery/board.yaml)
- [ADR-013 Multikernel Memory Protection](file:///docs/adr/ADR-013-multikernel-memory-protection-architecture.md)
- [EDGE32 Production Roadmap](file:///docs/roadmap/EDGE32-PRODUCTION-ROADMAP.md)
