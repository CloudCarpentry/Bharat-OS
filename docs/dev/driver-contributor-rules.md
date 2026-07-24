---
title: Driver Contributor Rules
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - dev
see_also:
  - README.md
---
# Driver Contributor Rules

## Purpose
This document provides explicit guidelines for contributors adding new drivers or subsystems to Bharat-OS.

## Where New Code Goes
- All hardware-specific control logic must go into `core/drivers/` (e.g., `core/drivers/bus/gpio/`, `core/drivers/led/`).
- Do **not** put hardware specific or driver policy logic inside `core/kernel/`. The kernel is mechanism-only (e.g., memory mapping, IRQ routing).
- User-space policy, orchestration, and permissions belong in `core/services/`.

## Driver Design Rules
1. **Lifecycle**: Every driver must implement explicit `probe()` and `remove()` functions. Cleanup on failure must be idempotent, and no state should leak after removal.
2. **Ownership**: Maintain single ownership of mapped resources, IRQ registrations, and DMA buffers. Do not use implicit global mutable state.
3. **Errors**: Handle errors rigorously. Never swallow a failed hardware initialization silently during `probe()`.
4. **Power Management**: At minimum, stub `suspend()` and `resume()` callbacks. Provide safe defaults upon resuming.
5. **Event Emission**: Ensure state changes (add, remove, fault) emit events to the driver core for `devmgr` consumption.

## Required Tests
- Probe and match tests.
- Invalid-device error paths.
- Remove and failure cleanup.
- Subsystem-specific functional tests (e.g., bus transfers).

## Build Gating
- New drivers must be gated behind appropriate CMake options (e.g., `BHARAT_ENABLE_NEW_DEVICE`). This ensures minimal target profiles only compile the code they require.