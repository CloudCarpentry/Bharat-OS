---
title: Boot Flow: RISC-V (64-bit)
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - boot
see_also:
  - README.md
---
# Boot Flow: RISC-V (64-bit)

## Overview

Unlike x86, RISC-V enforces strict privilege levels (M-Mode, S-Mode, U-Mode). The Bharat-OS microkernel operates strictly in Supervisor Mode (S-Mode), offloading machine-level hardware initialization to the Supervisor Binary Interface (SBI) firmware.

## Sequence

```mermaid
sequenceDiagram
    participant Hardware as BootROM (M-Mode)
    participant SBI as SBI Firmware (M-Mode)
    participant Stub as Assembly Stub (_start) (S-Mode)
    participant Common as Kernel (kernel_boot.c)
    participant User as Root User Task

    Hardware->>SBI: Bring up HART, init RAM
    SBI->>SBI: Setup trap delegation
    SBI->>SBI: Discover hardware (DeviceTree)
    SBI->>SBI: Load kernel binary
    SBI->>Stub: mret (Drop to S-Mode)
    Stub->>Stub: Setup supervisor stack (sp)
    Stub->>Stub: Capture a0 (hartid), a1 (fdt_ptr)
    Stub->>Stub: FDT Adapter normalizes to boot_info_t (BOOT_SOURCE_OPENSBI_FDT)
    Stub->>Common: Call kernel_main_common(boot_info)
    Common->>Common: boot_validate_all(boot_info)
    Common->>Common: boot_mode_resolve(boot_info)
    Common->>Common: boot_common_early(boot_info)
    Common->>Common: boot_common_memory(boot_info)
    Common->>Common: Configure trap vectors (stvec)
    Common->>Common: Wake secondary HARTs (sbi_send_ipi)
    Common->>Common: boot_common_runtime(boot_info)
    Common->>User: Handover capabilities & Start
```

1. **Hardware / BootROM (M-Mode)**: Initial silicon brings up the primary HART (Hardware Thread), initializes RAM, and jumps to the SBI firmware.
2. **SBI Firmware (OpenSBI / RustSBI) (M-Mode)**:
   - Sets up trap delegation so traps pass immediately down to the OS.
   - Discovers the hardware topology (DeviceTree).
   - Loads the Bharat-OS kernel binary into memory.
   - Issues an `mret` instruction, dropping privileges to S-Mode and jumping to the kernel's `_start`.
3. **Assembly Stub (`_start`) (S-Mode)**:
   - Sets up the supervisor stack (`sp`).
   - Captures the HART ID (`a0`) and DeviceTree pointer (`a1`) passed implicitly by the firmware.
   - An architecture-specific boot source adapter (e.g. `BOOT_SOURCE_OPENSBI_FDT`) parses the Flattened Device Tree (FDT) and populates the canonical `boot_info_t` structure.
   - Calls the generic, C-level `kernel_main_common(boot_info_t)`.
4. **Microkernel Initialization (`kernel_boot.c`)**:
   - `boot_validate_all(boot_info)` validates the standardized structure.
   - `boot_mode_resolve(boot_info)` determines the runtime mode (e.g., `BOOT_MODE_NORMAL`, `BOOT_MODE_RECOVERY`).
   - Initializes the PMM (`mm_pmm_init`) and VMM, configuring the `satp` register for Sv39/Sv48 paging.
   - Configures S-Mode trap vectors (`stvec`).
   - Wakes up secondary HARTs (Multicore Boot).
   - Calls `boot_common_runtime` to either run self-tests, start diagnostic applications, or launch the Root User Task depending on the resolved mode.
5. **Root Task Handover**: The capability system takes over execution, proceeding in a unified flow regardless of the underlying boot architecture.


## Shakti BSP baseline (E/C/I class)

Current code includes a board-profile baseline used during early S-mode bring-up:

- `BHARAT_RISCV_SOC_PROFILE` selects `qemu-virt`, `shakti-e`, `shakti-c`, or `shakti-i` at configure time.
- `hal_init()` resolves a profile-specific BSP descriptor (UART/PLIC/CLINT/DRAM ranges) and stores it as active board config.
- The `BOOT_SOURCE_OPENSBI_FDT` adapter now consumes OpenSBI boot arguments, translates them to the canonical `boot_info_t`, and hands them over to the generic tracking mechanism instead of directly passing them around.

This is intentionally a transition layer while full DTB parsing and page-table-backed MMIO mapping are completed.
