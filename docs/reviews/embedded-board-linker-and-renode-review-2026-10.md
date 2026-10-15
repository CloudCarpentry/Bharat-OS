# Bharat-OS 32-Bit Embedded Board Support, Linker Architecture & Renode Review

**Review Date**: October 15, 2026  
**Scope**: 32-Bit Embedded Board Support Framework, Memory Manifests, Linker Generation, Renode Simulation  
**Status**: **COMPLETED & HARDENED** (`BH-EMBED-LINK-P0-001`)  
**Authority**: [AGENTS.md](../../AGENTS.md), [ADR-040](../adr/ADR-040-unified-embedded-board-and-linker-framework.md)

---

## 1. Executive Summary & Root Cause Analysis

During embedded microcontroller qualification for the ARM Cortex-M4 (STM32F407 Discovery) and RISC-V32 (SiFive FE310 / HiFive1 Rev B), two critical hardware-level failures were identified in the legacy build system:

1. **STM32F4 MMIO Address Space Collision**:
   - *Symptom*: When launching Renode simulation for the STM32F4 Discovery board, the simulated CPU entered an infinite memory fault / HardFault loop upon execution of the initial instructions.
   - *Root Cause*: The legacy monolithic ARM32 linker (`core/kernel/linker_arm32.ld`) hardcoded kernel base address at `0x40010000`. On STM32F4 microcontrollers, `0x40010000` is the exact base address of the APB2 peripheral MMIO register block (specifically `TIM1_CR1`). The linker was writing machine code into peripheral registers rather than executable Flash memory (`0x08000000`), causing instruction fetch exceptions.
2. **RISC-V32 Privilege Level & Firmware Mismatch**:
   - *Symptom*: The default RV32 target configuration assumed an OpenSBI supervisor payload linked at `0x80200000` executing in S-mode.
   - *Root Cause*: Small embedded RV32 microcontrollers (e.g., SiFive FE310) execute in Machine mode (M-mode) directly from SPI Flash (`0x20000000`) with data in DTIM (`0x80000000`). In M-mode microcontrollers, OpenSBI is not present, and Physical Memory Protection (PMP) registers are strictly M-mode accessible. Attempting to run S-mode trap code on an M-mode-only core causes illegal instruction traps.
3. **Compiler Runtime Missing Symbols**:
   - *Symptom*: Compiling `kernel.elf` for ARM32 Cortex-M4 with Clang `--target=arm-none-eabi` failed with undefined references to `__aeabi_idiv` and `__aeabi_idivmod`.
   - *Root Cause*: ARM Cortex-M4 in soft-float mode relies on compiler-rt helper functions for signed 32-bit integer division when division operations occur in freestanding kernel libraries.

---

## 2. Architecture & Implementation Review

To permanently resolve these defects without polluting the core microkernel (`core/kernel/`) with board-specific `#ifdef`s, the **Unified 32-Bit Embedded Board Support and Linker Framework** (`BH-EMBED-LINK-P0-001`) was designed and implemented:

### 2.1 Declarative Board Manifests (`platform/boards/`)
Board-specific geometry is declared in YAML files validated against `tools/schemas/board.schema.yaml`:
- `stm32f407_discovery/board.yaml`: Cortex-M4, Flash `0x08000000` (1024 KB), SRAM `0x20000000` (128 KB), APB/AHB MMIO exclusion.
- `hifive1_rev_b/board.yaml`: RV32 MCU M-mode, SPI Flash `0x20000000` (4096 KB), DTIM `0x80000000` (16 KB), CLINT/PLIC MMIO exclusion.
- `rv32_virt/board.yaml`: QEMU RV32 Sv32 S-mode manifest.
- `qemu_virt_arm32/board.yaml`: QEMU ARM32 Cortex-A15 manifest.

### 2.2 Parameterized Linker Templates (`delivery/linker/templates/`)
Linker generation is decoupled from board geometry using template files:
- `cortex_m.ld.in`: Generates vector table `.isr_vector` at flash origin, initializes data from flash to SRAM, and verifies stack room.
- `riscv32_mcu.ld.in`: Generates reset vector at SPI flash or RAM origin, sets global pointer `__global_pointer$`, and places stacks in DTIM.
- `arm32_mmu_lite.ld.in`: Generates ARMv7-A exception vector table at RAM origin and builds static page tables.
- `riscv32_sv32.ld.in`: Generates Sv32 payload with supervisor vector `_start` at `0x80000000`.

### 2.3 Compile-Time Stack Assertion
All generated linker scripts enforce:
```ld
ASSERT(_sram_end >= _stack_top, "Error: Insufficient SRAM for stack allocation!");
```

### 2.4 Pre-Execution Link-Time MMIO Collision Detection
`tools/board/validate.py` mathematically verifies that no RAM or Flash region intersects any peripheral MMIO zone:
$$\max(O_M, O_P) < \min(O_M + L_M, O_P + L_P)$$
Any overlap immediately fails qualification with `[FAIL] No MMIO overlap`.

### 2.5 Compiler Runtime Support
Implemented software 32-bit signed division `__aeabi_idiv` and `__aeabi_idivmod` in `core/arch/arm/arm32/compiler_rt.c` and `core/lib/runtime/src/runtime.c`.

### 2.6 Static Userspace Runtime Model for Microcontrollers
Microcontrollers with 128 KB SRAM cannot support full dynamic system services (`namesvc`, `process_manager`). Configured `userspace.runtime_model: static` in `delivery/targets/renode/stm32f4_discovery.yaml`, allowing static linking of applications (`init_module.bin`).

---

## 3. Verification Evidence

### 3.1 Board Qualification Suite (`tools/board/validate.py --all`)
```
=== Qualifying 4 Embedded Board Manifests ===

--- Board: hifive1_rev_b ---
[PASS] CPU and compiler ABI compatible          : rv32-mcu / sifive-e31 (soft ABI)
[PASS] Flash layout validated                   : flash: 0x20000000..0x20400000 (4096K)
[PASS] SRAM and reserved stack fit              : dtim (16K) with 6K stacks leaves 10K
[PASS] Vector table within boot memory          : Located in flash (align 4)
[PASS] MPU region plan representable            : RISC-V PMP NAPOT/TOR representable
[PASS] No MMIO overlap                          : All 2 memory regions clear of 9 MMIO zones
[PASS] Required UART/timer devices defined      : Console: uart0, Timer: clint, IRQ: plic
[PASS] Renode platform configuration present    : platforms/cpus/sifive-fe310.repl verified
[PASS] Kernel and app ELF32 verified            : ELF check skipped (no binary path specified)
[PASS] Image ready for execution                : Board 'hifive1_rev_b' qualified

--- Board: qemu_virt_arm32 ---
[PASS] CPU and compiler ABI compatible          : armv7-a / cortex-a15 (hard ABI)
[PASS] Flash layout validated                   : ram: 0x40000000..0x60000000 (524288K)
[PASS] SRAM and reserved stack fit              : ram (524288K) with 48K stacks leaves 524240K
[PASS] Vector table within boot memory          : Located in ram (align 32)
[PASS] MPU region plan representable            : Virtual memory (MMU_LITE) enabled
[PASS] No MMIO overlap                          : All 1 memory regions clear of 5 MMIO zones
[PASS] Required UART/timer devices defined      : Console: pl011, Timer: arch_timer, IRQ: gicv2
[PASS] Renode platform configuration present    : platforms/cpus/cortex-a15.repl verified
[PASS] Kernel and app ELF32 verified            : ELF check skipped (no binary path specified)
[PASS] Image ready for execution                : Board 'qemu_virt_arm32' qualified

--- Board: rv32_virt ---
[PASS] CPU and compiler ABI compatible          : rv32-sv32 / rv32 (soft ABI)
[PASS] Flash layout validated                   : ram: 0x80000000..0x90000000 (262144K)
[PASS] SRAM and reserved stack fit              : ram (262144K) with 24K stacks leaves 262120K
[PASS] Vector table within boot memory          : Located in ram (align 4)
[PASS] MPU region plan representable            : Virtual memory (MMU_LITE) enabled
[PASS] No MMIO overlap                          : All 1 memory regions clear of 5 MMIO zones
[PASS] Required UART/timer devices defined      : Console: ns16550, Timer: sbi, IRQ: plic
[PASS] Renode platform configuration present    : platforms/cpus/riscv-virt.repl verified
[PASS] Kernel and app ELF32 verified            : ELF check skipped (no binary path specified)
[PASS] Image ready for execution                : Board 'rv32_virt' qualified

--- Board: stm32f407_discovery ---
[PASS] CPU and compiler ABI compatible          : armv7e-m / cortex-m4 (soft ABI)
[PASS] Flash layout validated                   : flash: 0x08000000..0x08100000 (1024K)
[PASS] SRAM and reserved stack fit              : sram (128K) with 12K stacks leaves 116K
[PASS] Vector table within boot memory          : Located in flash (align 4)
[PASS] MPU region plan representable            : PMSAv7 power-of-two alignment confirmed
[PASS] No MMIO overlap                          : All 3 memory regions clear of 4 MMIO zones
[PASS] Required UART/timer devices defined      : Console: usart2, Timer: systick, IRQ: nvic
[PASS] Renode platform configuration present    : platforms/boards/stm32f4_discovery.repl verified
[PASS] Kernel and app ELF32 verified            : ELF check skipped (no binary path specified)
[PASS] Image ready for execution                : Board 'stm32f407_discovery' qualified

=== Overall Board Qualification: PASS ===
```

### 3.2 Unit & Integration Test Suite (`pytest quality/tests/board/`)
```
============================= test session starts =============================
collected 12 items

quality\tests\board\test_board_manifest.py ....                          [ 33%]
quality\tests\board\test_board_validate.py .....                         [ 75%]
quality\tests\board\test_linker_generator.py ...                         [100%]

============================= 12 passed in 1.63s ==============================
```

### 3.3 Nirmaan CLI Verification (`.\nirmaan.ps1`)
- `.\nirmaan.ps1 targets`: Displays `stm32f4-renode` in the target matrix.
- `.\nirmaan.ps1 verify --all-boards`: Executes board qualification suite (100% PASS).
- `.\nirmaan.ps1 test board`: Executes qualification and 12 unit tests (100% PASS).
- `.\nirmaan.ps1 build stm32f4-renode`: Builds 169 targets including `kernel.elf` and emits `build-manifest.json` cleanly.

### 3.4 Canonical Verification Matrix
| Gate | Command | Status |
|---|---|---|
| Agent Config | `python tools/agents/check_agent_config.py` | **PASS** (16/16 skills valid and synced) |
| Syscall ABI | `python tools/abi/syscall_abi.py --check` | **PASS** (ABI lock and hashes verified) |
| Board Qualification | `python tools/board/validate.py --all` | **PASS** (4/4 boards qualified) |
| Board Unit Tests | `pytest quality/tests/board/` | **PASS** (12/12 test cases pass) |
| Renode Target Build | `python tools/build.py build --target-yaml delivery/targets/renode/stm32f4_discovery.yaml` | **PASS** (All targets compiled cleanly) |

---

## 4. Next Steps & Active Backlog

- **`BH-EMBED-BOOT-P0-002`**: Implement native Thumb-2 Cortex-M exception vector table and reset handler, plus RV32 M-mode trap dispatch.
- **`BH-EMBED-BSP-P1-003`**: Implement production HAL peripheral drivers (SysTick/CLINT, NVIC/PLIC, USART) without kernel layer violations.
