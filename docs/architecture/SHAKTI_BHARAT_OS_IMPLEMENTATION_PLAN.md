# SHAKTI Integration Plan for Bharat-OS

This document defines a **clean-integration plan** for SHAKTI support in Bharat-OS without contaminating architecture-neutral kernel surfaces.

## Goals

Bharat-OS must:

1. Boot natively on SHAKTI targets with SHAKTI-style bring-up and debug flows.
2. Reuse SHAKTI-SDK developer expectations (board selection, build/debug/upload habits) with minimal app changes.
3. Keep x86_64, arm64, and generic riscv64 clean and unaffected.

## Non-goals and guardrails

Do **not**:

- hardcode SHAKTI board maps into scheduler/mm/ipc.
- couple capability or IPC model to BSP conventions.
- spread board `#ifdef` logic through generic HAL layers.
- mirror SHAKTI-SDK repository layout inside core kernel paths.

Core invariants that must remain stable:

- capability ABI and rights checks;
- synchronous IPC and URPC semantics;
- scheduler contracts;
- VMM/PMM object model;
- personality-neutral syscall/kernel contract.

## Layered seam (required)

```text
Bharat-OS Core
  -> architecture-neutral kernel primitives
  -> architecture-specific HAL
  -> SHAKTI platform backend
  -> optional SHAKTI-SDK compatibility shim
```

SHAKTI support belongs in HAL/platform/backends, not in core kernel semantics.

## Repository shape and ownership

Recommended structure:

```text
kernel/
  arch/
    riscv64/
      core/
      generic/
      shakti/
        boot/
        trap/
        timer/
        irq/
        mm/
        smp/
drivers/
  interrupt/
    plic_shakti.c
    clint_shakti.c
  serial/
    uart_shakti.c
platform/
  shakti/
    boards/
    common/
compat/
  shakti_sdk/
    include/
    bsp_shim/
    make/
```

Existing Bharat-OS directories already align with this split (`kernel/src/hal`, `drivers/`, `tools/boards/`, `docs/`). Implementation should preserve that separation.

## Platform contract (Phase 0)

Introduce/standardize a platform contract used by generic RISC-V code:

- early console init/write;
- timer init/program/ack;
- irq init/mask/unmask/eoi;
- board memory map discovery;
- boot info parsing;
- hart startup hooks;
- MMIO reservation;
- reset/poweroff/panic hooks.

Suggested config flags:

- `CONFIG_ARCH_RISCV64`
- `CONFIG_PLATFORM_SHAKTI`
- `CONFIG_SHAKTI_BOARD_ARTIX7_35T`
- `CONFIG_SHAKTI_BOARD_ARTIX7_100T`
- `CONFIG_SHAKTI_SDK_COMPAT`
- `CONFIG_SHAKTI_SPI_BOOT`
- `CONFIG_SHAKTI_DEBUG_BOOT`

## Native SHAKTI bring-up backend (Phase 1)

### 1) Entry split

Two-stage boot entry:

- **Stage A (SHAKTI stub):** reset entry, stack, `.bss`, temporary traps, early UART, board discovery, boot metadata.
- **Stage B (generic kernel entry):** consume `bharat_boot_info`, continue generic bootstrap without board constants.

### 2) Board manifest system

Create canonical `bharat_board_desc` for SHAKTI boards with:

- board/SoC ID, hart count, ISA details;
- ROM/RAM ranges;
- UART/CLINT/PLIC base + IRQ/clock metadata;
- SPI/QSPI/I2C/GPIO/PWM capability flags;
- flash layout offsets and boot/debug mode flags.

### 3) Early console and panic path

Must provide panic-safe, heap-independent UART output:

- `early_putc`, `early_puts`;
- fatal trap/panic dump path;
- optional post-init ring buffer.

### 4) Trap/CLINT/PLIC

Implement SHAKTI backend handlers for:

- early trap vector setup;
- interrupt/exception split and cause decode;
- CLINT timer + software interrupts;
- PLIC priority/enable/claim/complete.

Generic callers should use neutral APIs (for example `platform_irq_init()` and `platform_timer_program()`).

### 5) Memory/bootstrap and linker layout

Implement board-manifest-driven memory init:

- bootstrap bump allocator;
- PMM seeding from descriptor;
- reserved regions (ROM, flash staging, MMIO, stacks, vectors).

Provide linker classes for:

- SRAM/BRAM minimal modes;
- DDR-backed modes;
- SPI-uploaded image format.

## SHAKTI-style workflow compatibility (Phase 2)

To minimize migration friction, Bharat-OS should expose workflow-compatible wrappers:

- `tools/shakti/list-targets`
- `tools/shakti/build`
- `tools/shakti/upload`
- `tools/shakti/debug`

These wrappers should map to existing Bharat build/upload/debug machinery rather than forking architecture.

## SHAKTI-SDK compatibility shim (Phase 3)

Create optional shim under `compat/shakti_sdk/` with familiar BSP-like names at boundary, while internal kernel/driver APIs remain Bharat-native.

Initial shim surface:

- UART/GPIO/SPI/I2C/timer/IRQ headers;
- thin wrapper C files in `bsp_shim/`;
- explicit scope statement: low-change for simple examples, moderate changes for BSP-internal-heavy apps.

## Firmware personality mode

Add/maintain lightweight profile for bare-metal-style SHAKTI use:

- no heavy POSIX stack;
- static-memory-capable mode;
- direct timer/irq/device access with minimal services.

## Device roadmap

Wave 1:

- UART, CLINT, PLIC, SPI/QSPI, GPIO, basic I2C, timer services, flash R/W/E.

Wave 2:

- PWM, watchdog, RTC, XADC, Ethernet-lite, pinmux.

## Multi-architecture safety rules

- Keep `riscv64/generic` distinct from `riscv64/shakti`.
- Never add SHAKTI conditionals into x86_64/arm64 paths.
- Keep personalities platform-neutral.

## Validation plan

### Bring-up matrix

Per board/target:

- debug boot;
- SPI boot;
- UART and panic path;
- timer tick and IRQ delivery;
- memory init sanity;
- scheduler/context-switch smoke.

### Compatibility matrix

Port SHAKTI-style demos:

- UART hello;
- timer interrupt;
- GPIO demo;
- SPI flash read;
- I2C sensor stub;
- benchmark sample.

### Regression matrix

Run Bharat tests for:

- x86_64;
- arm64;
- generic riscv64;
- riscv64 + SHAKTI backend.

## Milestones

1. **Board brings up**: early UART, traps, CLINT/PLIC, scheduler init logs.
2. **Image lifecycle works**: build/upload/boot with debug and SPI flows.
3. **SDK seam exists**: compat headers/wrappers + first migrated examples.
4. **Kernel stays clean**: no SHAKTI leakage into core APIs; cross-arch green.
5. **Ecosystem credibility**: docs, board matrix, porting guide, known limits.

## Recommended implementation order

1. Board descriptors + early UART
2. Trap + CLINT + PLIC
3. Boot info handoff + linker layouts
4. Flash/debug workflow
5. Minimal firmware profile
6. Compatibility shim
7. Example migration
8. Broader device drivers and tuning
