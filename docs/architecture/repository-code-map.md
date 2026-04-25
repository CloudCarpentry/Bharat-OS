---
title: Architecture-to-Code Mapping
status: Active
version: 1.0
owner: Architecture Team
last_updated: 2026-04-25
tags:
  - architecture
  - repository
  - code-map
  - docs-consolidation
---

# Architecture-to-Code Mapping

This document connects architecture documentation to the current code layout, so design docs can be validated quickly against implementation paths.

## Repository roots and responsibilities

| Repository path | Primary responsibility | Key architecture docs |
| --- | --- | --- |
| `core/arch/` | ISA-specific kernel/bring-up code (`x86`, `arm`, `riscv`, etc.) | `docs/architecture/boot/*`, `docs/architecture/arch-capability-matrix.md`, `docs/architecture/hardware-abstraction-and-drivers-baseline.md` |
| `core/boot/` | Common boot-time orchestration and handoff contracts | `docs/architecture/boot/BOOT_ARCHITECTURE.md`, `docs/architecture/boot/boot_contract.md`, `docs/architecture/core/boot-runtime-lifecycle.md` |
| `core/kernel/` | Core mechanism layer (scheduling, tasks/threads, IPC/URPC, isolation contracts) | `docs/architecture/kernel/*`, `docs/architecture/kernel-object-model.md`, `docs/architecture/status-code-contract.md` |
| `core/hal/` | Hardware abstraction interfaces and arch-facing adaptation | `docs/architecture/hardware-abstraction-and-drivers-baseline.md`, `docs/architecture/pr-spec-driver-baseline.md` |
| `core/platform/` | Board/SoC integration and platform wiring | `docs/architecture/device-profiles.md`, `docs/architecture/device-profiles-and-use-cases.md` |
| `core/drivers/` | Driver implementations and model evolution | `docs/architecture/driver-model.md`, `docs/architecture/components/drivers-subcomponents-architecture.md` |
| `core/services/` | Userspace/system service policy plane | `docs/architecture/components/services-subcomponents-architecture.md`, `docs/architecture/system/shell-architecture.md` |
| `core/personalities/` | ABI/personality compatibility layers | `docs/architecture/personalities/*`, `docs/architecture/contracts/personality-contract-architecture.md` |
| `core/stacks/` | Cross-domain composed stacks (network/storage/system profiles) | `docs/architecture/network-architecture.md`, `docs/architecture/storage-and-sandbox-strategy.md` |
| `core/lib/` | Shared internal libraries/utilities | `docs/architecture/shared/lib_architecture.md`, `docs/architecture/shared/lib_roadmap.md` |
| `interface/idl/` | Interface definition contracts for cross-component APIs | `docs/architecture/core/uapi-idl-ipc-boundary.md`, `docs/architecture/contracts/bidl-language-spec.md` |
| `interface/uapi/` | User-visible ABI and syscall-facing API surfaces | `docs/architecture/uapi/native_abi.md`, `docs/architecture/syscall/syscall-abi-boundary.md` |
| `interface/sdk/` | SDK-facing exported interface definitions | `docs/architecture/sdk-libc-architecture.md`, `docs/architecture/sdk-libc-plan.md` |
| `quality/tests/` | Unit/integration/e2e validation matrix | `docs/testing/e2e-testing.md`, `docs/testing/run-matrix.md`, `docs/architecture/verification-scope.md` |
| `tools/` | Build, run, flash, lint, ABI tooling, CI helpers | `docs/architecture/core/build-packaging-run-flash-debug-contract.md`, root `BUILD.md` |
| `delivery/` | Toolchains, target descriptors, release manifests/assets | `docs/architecture/placement.md`, `docs/architecture/device_class_mapping.md` |

## Quick integrity checks for documentation updates

When updating architecture docs:

1. Link to at least one concrete path under `core/`, `interface/`, `quality/`, `tools/`, or `delivery/`.
2. Avoid references to legacy root aliases if a `core/*` canonical path exists.
3. If a decision changes a boundary, update `docs/adr/` and this map in the same PR.
4. Move superseded material to `docs/archive/` instead of deleting history.

## Known compatibility aliases

Some docs may still mention historical alias roots (for example `kernel/`, `drivers/`, `platform/`, `tests/`) from before consolidation. Canonical ownership is under:

- `core/kernel/`
- `core/drivers/`
- `core/platform/`
- `quality/tests/`

Use canonical paths in new/updated docs.
