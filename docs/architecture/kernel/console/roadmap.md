---
title: Console Subsystem Roadmap
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# Console Subsystem Roadmap

This roadmap tracks the functional implementation of the Bharat-OS Console Subsystem.
When updating roadmap.md, mark items as complete only if the full documented guarantee is met. If partial progress is made, explicitly document the current state and any remaining structural limitations.

## Phase 1: Core Primitives & Early Boot Sinks (Completed)
- [x] Basic ring buffer (`console_buffer.c`)
- [x] Lockless panic mode routing (`console_panic.c`)
- [x] Memlog virtual backend (`memlog_console.c`)
- [x] Early/Runtime phase transitions (`console_core.c`)
- [x] Standard format parsing bounds (`console_vlog`)
- [x] Initial hardware drivers (NS16550, PL011)

## Phase 2: Heterogeneous Hardware Expansion (In Progress)
- [x] Architecture document updated to reflect broad hardware IPs and capability selection.
- [x] Structural driver skeletons added for `SiFive`, `Cadence`, `LPUART`, and `DW_APB` UART IPs.
- [x] CMake Component Policy integrated for per-profile and per-architecture driver selection.
- [ ] *Deferred:* Full register-level implementation of `SiFive`, `Cadence`, `LPUART`, and `DW_APB` drivers (currently implemented as stubs).
- [ ] *Deferred:* Dynamic DTB/ACPI parsing for runtime UART discovery across all architectures.

## Phase 3: Capabilities & Userspace Contract (In Progress)
- [x] `console_v1.bidl` interface defined for cross-boundary console operations.
- [ ] Userspace console daemon fully implements BIDL skeleton for URPC TTY multiplexing.
- [ ] Capability authorization hooks for `console_v1` endpoints.
- [ ] Framebuffer rendering backend transition to secure userspace display server (GUI strategy alignment).

## Phase 4: Hardening & SMP Concurrency
- [ ] Bounded spinlock validations during `CONSOLE_PHASE_RUNTIME`.
- [ ] Per-CPU non-maskable interrupt (NMI) panic buffering to prevent console log interleaving on panic.
- [ ] Lockless ring buffer implementation replacing the global `console_lock` during normal operation.