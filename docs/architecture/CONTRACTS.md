---
title: Bharat-OS Architecture and Interface Contract Index
status: Draft
owner: Architecture Team
reviewers: Core Maintainers
last_updated: '2024-01-01'
tags:
- doc
see_also:
- none
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
| Shell diagnostic control plane | `interface/include/bharat/uapi/shell/diagnostic.h` and ADR-023 | Native shell and capability-backed diagnostic providers | Shell diagnostic control-plane host test plus target smoke builds | System services maintainers |
| Processor trap-entry ABI | `docs/adr/ADR-020-mechanically-verified-trap-entry-abi.md` and `core/kernel/include/trap.h` | Build-generated `trap_offsets.inc`; five architecture entry stubs; normalized trap decoder | `python3 tools/abi/test_trap_frame_abi.py <generated>/trap_offsets.inc <five assembly files>` plus target smoke builds | Kernel architecture maintainers |
| Native syscall ABI | `interface/contracts/abi/native_syscalls.json` and ADR-018 | Build-generated syscall numbers/table metadata; native `write` bootstrap authority is the implicit current process | `python3 tools/abi/syscall_abi.py --check` regenerates twice in temporary directories and verifies the locked output hashes | Kernel ABI maintainers |
| Monotonic time UAPI | `interface/contracts/abi/native_syscalls.json`, `interface/include/bharat/uapi/time/time.h`, and ADR-026 | `BH_SYS_TIME_GET` exposes only the canonical monotonic nanosecond clock; unsupported clocks fail closed | ABI checker, BharatLibC backend tests, personality tests, and target smoke tests | Kernel ABI and runtime maintainers |
| Syscall compatibility lock | `interface/contracts/abi/native_syscalls.lock.json` | Locked syscall count, numbers, metadata, and generated-output SHA-256 values | ABI check and the intentional procedure in `docs/dev/native-syscall-abi-change.md` | Kernel ABI maintainers |
| Kernel configuration | `core/kernel/include/bharat_config.h.in` plus authoritative CMake/profile definitions | `build/<target>/generated/include/bharat_config.h` | Required target builds | Build + kernel maintainers |
| Root userspace runtime model | `interface/include/bharat/uapi/bootstrap/runtime_model.h`, `interface/include/bharat/uapi/bootstrap/root_launch.h`, and ADR-021 | Target resolver, boot-module packager, generic root loader, root userspace components | Runtime-model schema/resolver/packager tests plus five-target builds and smoke runs | Userspace architecture maintainers |
| Developer SDK v0.1 source boundary | `interface/sdk/include/bharat/`, `interface/sdk/README.md`, and ADR-009 | Hosted SDK libraries and future native UAPI/service bindings | Standalone header check, SDK host test, examples, and SDK install/consumer check | SDK and userspace runtime maintainers |
| Target and machine contracts | `delivery/targets/` and target matrix | Build/run manifests and emulator commands | Target build + smoke run | Platform maintainers |
| GUI input stream v1 | `interface/include/bharat/uapi/input/input_service.h` and ADR-034 | `inputmgr`, LVGL adapter, GUI clients, and QMP visual smoke harness | Wire layout assertions, GUI smoke host tests, x86_64 showcase build, and five-target smoke matrix | UI and device-service maintainers |
| Runtime implementation maturity | `interface/contracts/implementation_maturity.json` and ADR-022 | Build-time production maturity gate | `python3 tools/check_implementation_maturity.py --profile RELEASE` plus focused checker tests | Build + subsystem maintainers |
| Capability model | `core/kernel/include/capability.h`, `core/kernel/src/cap/cap_policy.c`, and ADR-002 | Process-owned CSpaces, kernel and service IPC entry points | Positive/negative capability, attenuation, revocation, and ownership tests | Security maintainers |
| IPC/uRPC wire contracts | Add exact IDL/header/ADR references | Kernel, monitor, services | Layout assertions, retry/replay/timeout tests | Kernel IPC maintainers |
| Kernel heap and NUMA fault policy | `docs/adr/ADR-023-kernel-heap-and-numa-fault-policy.md`, `core/kernel/include/bharat_config.h.in`, and `core/kernel/include/numa.h` | PMM, scheduler, VM object faults, MMU/MMU-Lite/MPU profiles | Configuration rejection, focused NUMA/VM tests, five-target builds and smoke runs | Memory maintainers |
| Scheduler/per-core ownership | Add exact architecture/ADR references | Scheduler and cross-core commands | SMP ownership/migration tests | Scheduler maintainers |
| Service lifecycle | Add exact service contract/ADR references | Service manager and services | Event-loop/restart/watchdog tests | Service runtime maintainers |
| Diagnostic event and evidence ABI | `interface/uapi/diag/` and `contracts/evidence/` | Per-core rings, diagnostic collector, host evidence tooling | Host ABI/ring/parser/schema tests | Observability maintainers |
| Userspace CPU feature descriptor | `interface/uapi/runtime/cpu_features.h` | CRT publication and library-local ISA resolvers; system intersection unless affinity constrains execution | Header layout assertions and BharatLibC resolver tests | Runtime and architecture maintainers |

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

## BHCF heterogeneous memory and tensor contracts

- Native HMEM ABI authority: `interface/uapi/bharat/memory/hmem.h`.
- Kernel/HAL lifecycle and coherency contract: `docs/architecture/compute/BHCF-001-heterogeneous-memory.md`.
- Tensor SDK semantic contract: `docs/architecture/compute/BHCF-002-tensor-sdk.md`.
- Boundary decision: `docs/adr/ADR-024-hmem-tensor-boundary.md`.
- Domain values 0-4 remain ABI-compatible; new targets must fail closed for any
  domain they do not advertise through runtime discovery.
- HMEM capabilities are process-CSpace authorities. Owner-core registries do
  not equate CPUs with CSpaces.
