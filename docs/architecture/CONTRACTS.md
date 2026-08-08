---
title: Bharat-OS Architecture and Interface Contract Index
status: Draft
owner: Architecture Team
reviewers: Core Maintainers
---

# Bharat-OS Architecture and Interface Contract Index

This file is the human-readable lookup map for authoritative machine-readable contracts and architecture invariants. It does not replace the underlying contract files.

## Contract precedence

1. Machine-readable authority under `interface/contracts/`.
2. Accepted ADRs under `docs/adr/`.
3. Architecture documents under `docs/architecture/`.
4. Implementation and tests.

When implementation differs from an authority, treat it as a defect or an explicit migration—not as permission to invent a second contract.

## Contract registry

| Area | Authority | Generated outputs / consumers | Required validation | Owner |
|---|---|---|---|---|
| Native syscall ABI | `interface/contracts/abi/native_syscalls.json` and ADR-018 | Build-generated syscall numbers/table metadata; native `write` bootstrap authority is the implicit current process | `python3 tools/abi/syscall_abi.py --check` | Kernel ABI maintainers |
| Syscall compatibility lock | `interface/contracts/abi/native_syscalls.lock.json` | ABI compatibility checker | ABI check and explicit migration review | Kernel ABI maintainers |
| Kernel configuration | `core/kernel/include/bharat_config.h.in` plus authoritative CMake/profile definitions | `build/<target>/generated/include/bharat_config.h` | Required target builds | Build + kernel maintainers |
| Target and machine contracts | `delivery/targets/` and target matrix | Build/run manifests and emulator commands | Target build + smoke run | Platform maintainers |
| Capability model | Add exact authority/ADR reference | Kernel and service IPC entry points | Positive/negative capability tests | Security maintainers |
| IPC/uRPC wire contracts | Add exact IDL/header/ADR references | Kernel, monitor, services | Layout assertions, retry/replay/timeout tests | Kernel IPC maintainers |
| VM authority and backend semantics | Add exact architecture/ADR references | MMU, MMU-Lite, MPU backends | VM lifecycle and backend tests | Memory maintainers |
| Scheduler/per-core ownership | Add exact architecture/ADR references | Scheduler and cross-core commands | SMP ownership/migration tests | Scheduler maintainers |
| Service lifecycle | Add exact service contract/ADR references | Service manager and services | Event-loop/restart/watchdog tests | Service runtime maintainers |
| Diagnostic event and evidence ABI | `interface/uapi/diag/` and `contracts/evidence/` | Per-core rings, diagnostic collector, host evidence tooling | Host ABI/ring/parser/schema tests | Observability maintainers |

## Required entry contents

Every contract entry should identify:

- authority path,
- version and compatibility policy,
- owner/reviewer,
- producers and consumers,
- generated outputs,
- security/capability requirements,
- ownership/lifecycle rules,
- failure semantics,
- validation commands and evidence path,
- related ADRs.

## Update rule

Any PR changing an external interface, wire layout, public kernel API, ownership protocol, capability requirement, memory backend behavior, target definition, or required validation gate must update this index and the underlying authority/ADR in the same change.
