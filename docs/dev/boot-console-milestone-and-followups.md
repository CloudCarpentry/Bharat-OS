---
title: Boot-console milestone and follow-up task set (2026-04-15)
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - dev
see_also:
  - README.md
---
# Boot-console milestone and follow-up task set (2026-04-15)

## Milestone committed in this phase

This milestone closes the **canonical boot-console architecture refactor** as a separate unit of work.

### What is now implemented

- Canonical boot-console contract implemented.
- Single authority path enforced:
  - `platform` → canonical descriptor → generic discovery → descriptor-driven serial bind.
- Duplicate platform wiring removed.
- Boot marker validation tightened in QEMU runtime harness checks.

### Runtime validation status at this milestone

- ✅ x86_64: runtime markers validated in QEMU.
- ✅ arm64: runtime markers validated in QEMU.
- ✅ riscv64: runtime markers validated in QEMU.
- ⚠️ arm32: runtime-marker completeness still pending.
- ⚠️ riscv32: link/runtime completeness still pending.

> This milestone explicitly does **not** claim “all 5 arches production-ready”.

---

## Follow-up tasks (split out from architecture PR)

## Task A — Full QEMU runtime enforcement

**Goal:** make QEMU runtime checks strict and environment-complete.

### Scope

- Install/verify required QEMU system emulators in Linux CI/runtime environment.
- Fail hard when required emulators are missing (instead of silent drift).
- Run full explicit architecture/runtime matrix.
- Enforce required markers:
  - `BOOT: kernel_main reached`
  - `BOOT: pmm initialized`
  - `BOOT: vmm initialized`
  - `[BOOT] Runtime mode:`

### Done criteria

- CI/environment setup provisions required QEMU binaries.
- Missing emulator is a hard failure in enforcement mode.
- Matrix execution and marker gates are stable and documented.

## Task B — arm32 runtime debugging

**Goal:** identify exact failure stage for arm32 runtime-marker gap.

### Scope

- Run arm32 QEMU boot with full serial log capture.
- Identify last emitted boot marker.
- Classify failure as one of:
  - early console only,
  - early-to-runtime console transition,
  - MMU-lite transition,
  - boot sequence halt.

### Done criteria

- Reproducible arm32 failure log attached.
- Last known-good marker documented.
- Root-cause class identified and handed off to fix PR.

## Task C — riscv32 link/runtime completion

**Goal:** close completeness gaps without reopening boot-console architecture design.

### Scope

- Capture full undefined symbol / link failure list.
- Classify failures by owner:
  - arch
  - hal
  - runtime
  - boot
  - libc/compiler shim
- Fix only missing completeness gaps.

### Done criteria

- Link/runtime error inventory created and categorized.
- Focused fixes merged without expanding architecture scope.

## Task D — Canonical boot-console host tests

**Goal:** add host-side coverage for the canonical descriptor contract path.

### Test targets

- `platform_get_boot_console_desc()`
- `console_discover_devices()`
- `console_serial_register_device()`

### Required coverage

- Valid per-board descriptors.
- Invalid driver/type.
- No-UART case.
- Null/malformed descriptor.
- No rediscovery path.

### Done criteria

- Host tests added and passing in host test presets.
- Negative-path assertions block regressions in descriptor contract handling.
