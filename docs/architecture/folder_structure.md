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
- Mechanism (kernel/driverscore/hal) and policy (services/stacks/personalities) should stay separated.
- External contracts should be versioned and published through `interface/uapi/` + `interface/idl/` (legacy root aliases may exist during migration).

### Kernel algorithmic foundation directories

- `core/kernel/include/ds/` and `core/kernel/src/ds/`
  contain policy-free kernel data structures such as range indexes,
  sparse object indexes, intrusive lists, and bounded queues.

- `core/kernel/include/sync/` and `core/kernel/src/sync/`
  contain kernel synchronization primitives such as RCU-lite, sequence
  counters, barriers, and lock helpers.

- `core/kernel/include/verify/` and `core/kernel/src/verify/`
  contain verification helpers, checked arithmetic, hook validation,
  invariant helpers, and safety annotations.

## Current alignment snapshot (2026-04-24)

| Area | Target direction | Current alignment | Notes |
| --- | --- | --- | --- |
| `arch/` | ISA-specific implementation | Strong | Includes `arm`, `riscv`, `x86`, plus `xtensa`, `arc`, `shakti`. |
| `hal/` | abstraction contracts + generic glue | Partial | Contains arch-specific directories (`hal/arm64`, `hal/x86_64`, etc.), which blurs strict abstraction-only intent. |
| `core/platform/` | board/soc/machine integration | Strong | Canonical tree is `core/platform/*`; legacy `platform/` symlink remains during migration. |
| `kernel/` | minimal mechanism core | Partial | Core migration is active: canonical sources and headers now live under `core/kernel/{src,include}`; `kernel/src/*` and `kernel/include` compatibility symlink wrappers are retained while profile/subsystem policy extraction is still in progress. |
| `core/drivers/` | hardware driver implementations | Strong | Canonical tree is `core/drivers/*`; legacy `drivers/` symlink remains during migration. |
| `core/services/` | policy managers by domain | Partial | Canonical tree is `core/services/*`; migration from flat manager naming continues under the new root. |
| `core/personalities/` | compatibility/domain personalities | Strong | Canonical tree is `core/personalities/*`; legacy `personalities/` symlink remains during migration. |
| `core/stacks/` | composed cross-layer subsystems | Partial | Canonical tree is `core/stacks/*`; legacy `stacks/` symlink remains during migration. |
| `interface/uapi/` + `interface/idl/` + `interface/sdk/` | explicit contract surface | Partial | `idl`, `uapi`, and `sdk` are now under `interface/`; enforcement and include-surface migration remain in progress. |
| `quality/tests/` | test harnesses, host/unit/e2e suites | Active | Canonical host/unit/e2e test tree now lives under `quality/tests/`; legacy `tests` is retained as a compatibility symlink during migration cleanup. |

## Target structure (logical)

```text
Bharat-OS/
  core/
    arch/
    boot/
    hal/
    platform/
    kernel/
      include/
        ds/
        sync/
        verify/
      src/
        ds/
        sync/
        verify/
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
      storage/
        metadata/
    lib/
  interface/
    uapi/
    idl/
    sdk/
  quality/
    tests/
  delivery/
    assets/
    configs/
    targets/
    release/
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
