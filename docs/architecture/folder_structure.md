---
title: Bharat-OS Project Folder Structure
status: Active
version: 1.2
owner: Architecture Team
reviewers: Core Maintainers
last_updated: 2026-04-24
tags:
  - architecture
  - structure
  - boundaries
  - repository
---

# Bharat-OS Project Folder Structure

This document defines target folder boundaries and records the **current alignment snapshot** from the repository tree.

## Boundary rules

- One folder = one primary responsibility.
- Mechanism (kernel/drivers/hal) and policy (services/stacks/personalities) should stay separated.
- External contracts should be versioned and published through `interface/uapi/` + `interface/idl/` (legacy root aliases may exist during migration).

## Current alignment snapshot (2026-04-23)

| Area | Target direction | Current alignment | Notes |
| --- | --- | --- | --- |
| `arch/` | ISA-specific implementation | Strong | Includes `arm`, `riscv`, `x86`, plus `xtensa`, `arc`, `shakti`. |
| `hal/` | abstraction contracts + generic glue | Partial | Contains arch-specific directories (`hal/arm64`, `hal/x86_64`, etc.), which blurs strict abstraction-only intent. |
| `platform/` | board/soc/machine integration | Strong | `platform/common`, `platform/boards`, `platform/qemu` present. |
| `kernel/` | minimal mechanism core | Partial | Core migration is active: `kernel/src/{core,init,boot}` and `kernel/include` now resolve via `core/kernel/*` compatibility paths; profile/subsystem policy is still mixed in some paths. |
| `drivers/` | hardware driver implementations | Strong | Rich domain structure exists; some taxonomy overlaps remain (`block` vs `storage`, `class` vs `devices`). |
| `services/` | policy managers by domain | Partial | New `services/core|system|device|network` exists alongside legacy flat managers. |
| `personalities/` | compatibility/domain personalities | Strong | `compat/{linux,android,windows}` and `domain/automotive` present. |
| `stacks/` | composed cross-layer subsystems | Partial | Present (`network`, `can`, `ui`, `storage`) but ownership boundaries need tighter contracts. |
| `interface/uapi/` + `interface/idl/` + `interface/sdk/` | explicit contract surface | Partial | `idl`, `uapi`, and `sdk` are now under `interface/`; enforcement and include-surface migration remain in progress. |

## Target structure (logical)

```text
Bharat-OS/
  arch/
  boot/
  hal/
  platform/
  kernel/
  drivers/
  services/
    core/
    system/
    security/
    device/
    network/
  personalities/
    compat/
    domain/
    common/
  stacks/
  interface/
    uapi/
    idl/
    sdk/
  lib/
  tests/
  tools/
  docs/
```

## Migration priorities

1. **Service consolidation:** complete migration from flat `services/*mgr` directories to domain buckets (`core/system/device/network/security`).
2. **HAL boundary cleanup:** move or wrap architecture-specific HAL internals to preserve abstraction contract clarity.
3. **Kernel policy extraction:** migrate profile/personality-heavy policy from kernel paths into services/personalities where possible.
4. **Driver taxonomy finalization:** document and enforce ownership split for `block` vs `storage`, `class` vs device-specific drivers.
5. **Contract discipline:** require `interface/uapi` + `interface/idl`-backed interfaces for new cross-component APIs.

## Coding tasks identified from structure audit

- Create a phased rename/move plan for legacy services with compatibility headers and build aliases.
- Add CI lint that fails when new cross-layer interfaces are introduced without `interface/idl` or `interface/uapi` artifacts.
- Introduce architecture-boundary checks to prevent new policy code under `kernel/src/profile` unless explicitly approved.
- Add a driver ownership manifest (`drivers/registry`) mapping each driver to canonical subsystem and service owner.
