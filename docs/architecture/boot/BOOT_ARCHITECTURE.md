---
title: Bharat-OS Boot Architecture
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
# Bharat-OS Boot Architecture

## 1. Purpose

This document describes the Bharat-OS boot architecture, including:

* early boot flow
* boot phase ownership
* separation of boot/session policy from generic kernel subsystems
* use of boot mode, profile, personality, and hardware facts
* runtime entry selection after core initialization

## 2. Design goals

* keep `core/kernel/main.c` thin
* support multiple architectures and boards
* support headless and display-capable systems
* support production, diagnostic, recovery, manufacturing, and benchmark flows
* avoid hardcoded demo/test runtime behavior in the common boot path
* preserve compatibility during migration

## 3. Core concepts

### 3.1 Hardware facts

Hardware and platform discovery provide facts such as:

* serial/UART availability
* framebuffer/VGA availability
* MMU/MPU capability
* SMP/core count
* timers, interrupts, watchdogs

These are facts, not policy.

### 3.2 Profile

Profile defines the product/system class and default feature posture.

Examples:

* desktop
* server
* embedded_headless
* automotive
* datacenter
* network_appliance

### 3.3 Personality

Personality defines runtime behavior style.

Examples:

* debug
* release
* diagnostic
* secure_locked
* benchmark

### 3.4 Boot mode

Boot mode defines the selected boot-session path for the current boot.

Supported modes:

* normal
* diagnostic
* recovery
* manufacturing
* benchmark
* legacy_bringup

Boot mode is resolved from:

* explicit boot arguments
* build/personality fallback
* safe default policy

## 4. Boot layering

### 4.1 `main.c`

`core/kernel/main.c` is the thin kernel entry orchestrator.
Its responsibilities are limited to:

* receiving normalized boot info
* initializing CPU-local early state
* BSP/AP split
* calling staged common boot functions

It must not become a large policy engine.

### 4.2 Boot layer

The boot layer owns:

* boot info
* boot argument parsing
* boot mode selection
* boot-phase orchestration
* boot-time policy decisions
* recovery/bootstrap routing

### 4.3 Generic kernel subsystems

Generic subsystems remain outside the boot policy layer:

* PMM
* VMM
* scheduler
* IPC
* trap handling
* console core
* drivers
* services

## 5. Boot phase model

The common BSP boot flow is phase-based:

1. early boot
2. security/bootstrap setup
3. memory setup
4. core/platform/runtime services setup
5. runtime entry selection

This structure allows the kernel to remain readable and portable.

## 6. Runtime entry selection

After core boot phases are complete, the boot layer selects a boot mode and enters the corresponding runtime path.

### 6.1 Normal mode

Intended steady-state production-oriented path.
This mode should avoid direct hardcoded demo/test launches.

### 6.2 Diagnostic mode

Allows additional diagnostics, boot tests, and debug-oriented bring-up behavior.

### 6.3 Recovery mode

Provides a minimal and restricted path intended for fallback or recovery scenarios.

### 6.4 Manufacturing mode

Used for board validation, manufacturing checks, and controlled factory-style testing.

### 6.5 Benchmark mode

Used for explicit performance or benchmark-oriented sessions.

### 6.6 Legacy bring-up mode

Temporary compatibility mode preserving the previous hardcoded bring-up behavior while the boot architecture is being refactored.

## 7. Why boot mode belongs in the boot layer

Boot mode is not:

* an architecture concept
* a board concept
* a generic scheduler concept
* a generic kernel-wide mode flag

Boot mode is a boot-session routing decision, so it belongs in:

* `core/kernel/include/boot`
* `core/kernel/src/boot`

This keeps:

* architecture/board layers responsible for facts
* profile/personality responsible for defaults
* boot layer responsible for session selection

## 8. Console implications

The boot architecture must support:

* serial-first early boot
* optional framebuffer/VGA runtime presentation
* headless fallback
* future diagnostic shell/monitor integration

In the current migration phase, runtime mode selection is separated first; broader console policy remains a follow-up task.

## 9. Compatibility strategy

The legacy boot behavior is preserved through `LEGACY_BRINGUP` mode.
This prevents disruptive refactors while:

* new runtime modes are introduced
* test and demo behavior is isolated
* future init handoff becomes possible

`LEGACY_BRINGUP` is transitional and should not remain the permanent default.

## 10. Future work

Planned follow-up tasks include:

* staged boot health and self-test policy
* console policy framework
* minimal kernel monitor/shell
* init/service handoff
* failure routing and recovery policy
* steady-state runtime ownership cleanup

## 11. File ownership

* `core/kernel/main.c` — thin common kernel entry
* `core/kernel/src/kernel_boot.c` — phase orchestration
* `core/kernel/include/boot/boot_mode.h` — boot mode interface
* `core/kernel/src/boot/boot_mode.c` — boot mode selection logic

## 12. Summary

The Bharat-OS boot architecture separates:

* discovered hardware facts
* product/system profile
* behavior personality
* per-boot session mode

This improves portability, maintainability, and scalability across:

* x86_64
* arm32
* arm64
* riscv32
* riscv64
* different boards
* headless and graphical systems
* bring-up, production, recovery, and diagnostic use cases

## Boot policy note

Boot behavior is now driven through a boot-layer mode abstraction rather than hardcoded runtime demo/test execution. The boot layer resolves the current session path using boot arguments and policy fallback, while hardware/platform discovery and profile/personality remain separate concerns.

## Boot self-test policy

Boot health validation is stage-aware and mode-aware.

Stages:

* early
* security
* memory
* platform
* runtime

Policy classes:

* mandatory
* quick
* extended
* manufacturing
* benchmark-only

Normal boot runs only mandatory checks by default, while diagnostic and manufacturing boots allow richer validation.

This keeps production boot deterministic while still supporting bring-up, validation, and benchmarking flows.

## Canonical Boot Contract Architecture Update
As of Phase 1, Bharat-OS standardizes on `boot_info_t` under `boot/include/boot/`, deprecating ad-hoc parsers in `core/kernel/main.c`. Adapters (Multiboot2, FDT, OpenSBI) normalize all hardware-specific properties into this structure prior to early validation. See `docs/architecture/boot/` for details on validation and security posture handling.

## 13. Current Status & Roadmap

The current code status and future roadmap for the Bharat-OS boot architecture is detailed in `docs/architecture/boot/boot-status-and-roadmap.md`. Key ongoing tasks include fleshing out secure boot attestation, improving the `init` and `servicemgr` services from scaffold to production, and building out the memory and console policy frameworks.
