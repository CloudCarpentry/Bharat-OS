---
title: 32-Bit Embedded Board Support and Linker Framework Architecture
status: Accepted
owner: Architecture Working Group & Platform Team
last_updated: 2026-10-15
tags:
  - docs
  - architecture
  - bsp
  - linker
  - mpu
  - arm32
  - riscv32
  - renode
see_also:
  - ../adr/ADR-040-unified-embedded-board-and-linker-framework.md
  - arm32-rv32-edge-tier-plan.md
  - edge32-portability-audit.md
---

# 32-Bit Embedded Board Support and Linker Framework Architecture

## 1. Architectural Scope and Problem Statement

Bharat-OS targets a wide spectrum of computing platforms, from 64-bit multi-core NUMA application processors (`x86_64`, `arm64`, `riscv64`) down to 32-bit resource-constrained microcontrollers and edge SoCs (`arm32`, `riscv32`).

Prior to the introduction of this framework, 32-bit platforms used monolithic, hardcoded linker scripts (e.g. `core/kernel/linker_arm32.ld` placing code at `0x40010000`). This caused severe hardware failures:
1. **STM32F4 APB2 MMIO Collision**: On ARM Cortex-M4 targets (such as STM32F407 Discovery), `0x40010000` lies directly inside the APB2 peripheral MMIO address space (`TIM1`), corrupting hardware timers and triggering infinite CPU HardFault loops.
2. **RISC-V32 Privilege Level Mismatch**: The default RV32 linker script assumed an OpenSBI supervisor payload at `0x80200000`. On small bare-metal microcontrollers (such as SiFive FE310 / HiFive1 Rev B), execution must begin directly in Machine mode (M-mode) from SPI flash at `0x20000000`, because OpenSBI is absent and Physical Memory Protection (PMP) registers are strictly M-mode accessible.
3. **Monolithic Linker Proliferation**: Adding each new board previously required duplicating linker scripts, introducing subtle memory layout bugs and unchecked stack overflow risks.

The **Unified Embedded Board Support and Linker Framework** (`BH-EMBED-LINK-P0-001`) eliminates hardcoded assumptions by establishing declarative board manifests, parameterized linker templates, and pre-execution qualification tools.

---

## 2. Framework Architecture

The framework decouples board memory topology from kernel source code and build automation:

```
┌──────────────────────────────────────────────┐
│  platform/boards/<board_id>/board.yaml       │
│  - Architecture & CPU type                   │
│  - Memory regions (flash, sram, dtim, ram)   │
│  - Stack budgets (kernel, irq, fault)        │
│  - MMIO exclusion zones (APB, AHB, etc.)     │
│  - Peripherals (UART, timer, interrupt)      │
└──────────────────────┬───────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────┐       ┌────────────────────────────────────┐
│  tools/board/linker_generator.py             │◄──────┤ delivery/linker/templates/*.ld.in  │
│  - Parses declarative board manifest         │       │ - cortex_m.ld.in                   │
│  - Computes memory map & symbols             │       │ - riscv32_mcu.ld.in                │
│  - Emits linker script & layout JSON         │       │ - arm32_mmu_lite.ld.in             │
└──────────────────────┬───────────────────────┘       │ - riscv32_sv32.ld.in               │
                       │                               └────────────────────────────────────┘
                       ▼
┌──────────────────────────────────────────────┐
│  delivery/linker/generated/<board>.ld        │
│  - MEMORY block with accurate origins/lengths│
│  - Vectors placed in executable ROM/Flash    │
│  - Data copied from Flash LMA to SRAM VMA    │
│  - Compile-time stack overflow assertion     │
└──────────────────────────────────────────────┘
                       ▲
                       │
┌──────────────────────────────────────────────┐
│  tools/board/validate.py                     │
│  - 10-point automated qualification suite    │
│  - Link-time MMIO collision detection        │
│  - Vector table alignment verification       │
│  - Pre-boot simulator check                  │
└──────────────────────────────────────────────┘
```

---

## 3. The Four Hardware Execution Classes

Embedded boards are partitioned into four distinct hardware classes. Each class defines the CPU execution mode, memory protection mechanism, and vector table alignment:

| Hardware Class | Architecture | Privilege Model | Memory Protection | Vector Alignment | Reference Board |
|---|---|---|---|---|---|
| **`cortex_m`** | `armv7e-m` | Thread / Handler Mode | PMSAv7 / PMSAv8 MPU | 4-byte (word) | `stm32f407_discovery` |
| **`riscv32_mcu`** | `rv32-mcu` | Machine / User (M/U-mode)| RISC-V PMP (NAPOT/TOR) | 4-byte (word) | `hifive1_rev_b` |
| **`arm32_mmu_lite`** | `armv7-a` | PL1 SVC / PL0 User | Static/Eager VMSAv7 Pages | 32-byte (`VBAR`) | `qemu_virt_arm32` |
| **`riscv32_sv32`** | `rv32-sv32` | Supervisor / User | Sv32 2-level paging | 4-byte (`stvec`) | `rv32_virt` |

---

## 4. Declarative Board Manifest Authority

Every target board has an authoritative manifest in `platform/boards/<board_id>/board.yaml`. The schema is formally defined and validated by `tools/schemas/board.schema.yaml`.

### Manifest Schema Structure

```yaml
version: 1
board: stm32f407_discovery
display_name: "STMicroelectronics STM32F407 Discovery (Cortex-M4)"
hardware_class: cortex_m
architecture: armv7e-m
cpu: cortex-m4
abi:
  float_abi: soft
  eabi: eabi

memory:
  flash:
    type: flash
    origin: 0x08000000
    length: 0x00100000    # 1024 KB
    access: rx
    executable: true
  sram:
    type: sram
    origin: 0x20000000
    length: 0x00020000    # 128 KB
    access: rw
    executable: false

stacks:
  kernel_stack_bytes: 8192
  irq_stack_bytes: 2048
  fault_stack_bytes: 2048

vector_table:
  section: .isr_vector
  region: flash
  align: 4

mmio:
  apb1:
    origin: 0x40000000
    length: 0x00008000
  apb2:
    origin: 0x40010000
    length: 0x00008000

devices:
  console_uart:
    name: usart2
    base_address: 0x40004400
  monotonic_timer:
    name: systick
    type: core_timer
  interrupt_controller:
    name: nvic
    type: arm_nvic
```

---

## 5. Templated Linker Script Generation

Linker scripts are generated automatically from parameterized templates in `delivery/linker/templates/`:
- `cortex_m.ld.in`: Generates vector table `.isr_vector` at flash origin, initializes data from flash to SRAM, and verifies stack room.
- `riscv32_mcu.ld.in`: Generates reset vector at SPI flash or RAM origin, sets global pointer `__global_pointer$`, and places stacks in DTIM.
- `arm32_mmu_lite.ld.in`: Generates ARMv7-A exception vector table at RAM origin and builds static page tables.
- `riscv32_sv32.ld.in`: Generates Sv32 payload with supervisor vector `_start` at `0x80000000`.

### Compile-Time Stack Overflow Assertion
Every generated linker script includes a compile-time assertion that guarantees `.data`, `.bss`, and reserved stack budgets do not exceed physical SRAM:

```ld
ASSERT(_sram_end >= _stack_top, "Error: Insufficient SRAM for stack allocation!");
```

---

## 6. Pre-Execution Qualification Suite (`tools/board/validate.py`)

The qualification CLI executes 10 automated checks against any board manifest and compiled binary:

```
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
```

### MMIO Collision Prevention Principle
The validator calculates the intersection between each memory region $[O_M, O_M + L_M)$ and each MMIO exclusion zone $[O_P, O_P + L_P)$:
$$\max(O_M, O_P) < \min(O_M + L_M, O_P + L_P)$$
If this condition is true for any pair, the validator reports `[FAIL] No MMIO overlap` with the exact conflicting addresses and aborts the build immediately.

---

## 7. Developer Tooling Integration

The framework is fully integrated into both standard tools and the Nirmaan CLI:

```bash
# 1. Qualify all board manifests
nirmaan verify --all-boards
# Equivalent to: python tools/board/validate.py --all

# 2. Run board qualification unit and integration tests
nirmaan test board
# Equivalent to: pytest quality/tests/board/

# 3. Build a Renode target for an embedded board
nirmaan build stm32f4-renode

# 4. Generate linker script for a specific board
python tools/board/linker_generator.py --board stm32f407_discovery
```
