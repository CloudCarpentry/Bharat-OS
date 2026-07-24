---
title: Bharat-OS Console Subsystem Architecture
status: draft
owner: Divyang Panchasara
version: 1.1.0
last_updated: 2024-05-24
tags: [architecture, console, kernel, drivers, uart]
---

# Bharat-OS Console Subsystem Architecture

The Bharat-OS console subsystem provides a robust, production-baseline, capability-driven stack designed for early boot diagnostics, runtime structured formatting, and hardened panic modes across a highly heterogeneous hardware landscape. It spans multiple device profiles (Automotive, Drone, Medical, Edge, Mobile, Desktop, Datacenter) and architectures (ARM32/64, RISCV32/64, x86_64).

## Layering & Composition

1. **Console Core** (`core/kernel/include/console/`, `core/kernel/src/console/`):
   Manages global state, phase transitions (`early`, `runtime`, `panic`), formatting bounds (`console_vlog`), the central ring buffer (`console_buffer`), and routing to active backends. It is hardware-agnostic.
2. **Console Backends** (`memlog_console.c`, `serial_console.c`, `framebuffer_console.c`):
   Expose hardware or virtual sink behavior through a unified capability contract (`console_backend_ops_t`). The `memlog` backend acts as a reliable memory storage sink tightly coupled with the core ring buffer. The `serial` and `framebuffer` backends dispatch to specific hardware driver operations.
3. **Hardware Drivers / Renderers** (`core/drivers/serial/*/`, `console_render_fb.c`):
   Implement register-level manipulation and text/pixel rendering. Rather than monolithic drivers, Bharat-OS implements modular hardware IPs:
   * **NS16550:** Standard PC COM port (x86_64, QEMU virt)
   * **PL011:** ARM PrimeCell UART (ARM32, ARM64)
   * **SiFive UART:** Common RISC-V UART IP (RISCV32, RISCV64)
   * **Cadence UART:** Used in Xilinx Zynq / Automotive
   * **LPUART (Low Power UART):** NXP i.MX / Medical / Drones
   * **DW APB UART:** Synopsys DesignWare IP common in broad SoCs
   * **Framebuffer:** GUI and diagnostic display handoffs
4. **Discovery & Policy** (`console_discovery.c`, `console_policy.c`, `core/arch/*/console/arch_console_discovery.c`):
   Platform-specific stubs discover board hardware statically or via DTB/ACPI and report unified descriptors. The policy module selects the best sinks for early, runtime, and panic phases based on backend capabilities (`CON_CAP_*`).

## Capability-Driven Selection

Console backends self-report capabilities via `console_caps_t`:
* `CON_CAP_WRITE_POLL`: Backend can operate without interrupts (crucial for early boot/panic).
* `CON_CAP_PANIC_SAFE`: Backend promises deterministic, lock-free output during a kernel panic.
* `CON_CAP_VISIBLE_SINK`: Output reaches human-readable interfaces (Serial, FB), excluding silent memory logs.
* `CON_CAP_REPLAY_SAFE`: Capable of receiving the early-boot ring buffer replay during runtime transition.

## Panic Mode Resiliency

During a kernel panic (`console_enter_panic()`), standard locking (`console_lock`) is deliberately bypassed. Only backends advertising `CON_CAP_PANIC_SAFE` or flagged `panic_ok` receive logs. The router switches from calling `write()` to `write_atomic()`, forcing backends to bypass interrupts and rely strictly on deterministic polling routines.

## Hardware Support & Selection (CMake)

Due to the vast hardware differences between a server (`DATACENTER`) and a robotic edge node (`DRONE`), UART drivers are not compiled universally. Bharat-OS utilizes `BharatComponentPolicy.cmake` to select drivers. For instance:
* `BHARAT_PROFILE_DATACENTER` with `x86_64` defaults to `NS16550`.
* `BHARAT_PROFILE_AUTOMOTIVE_ECU` with `arm64` might default to `Cadence` or `DW APB`.
* `BHARAT_PROFILE_DRONE` might default to `LPUART`.

## Userspace Integration (BIDL)

The userspace console daemon (`core/services/console/main.c`) does not touch hardware registers directly. It communicates with the kernel and other subsystems via the deterministic `console_v1.bidl` contract. The daemon acquires a hardware output capability from the capability manager and multiplexes TTY streams over URPC.
