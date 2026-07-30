---
name: bharat-os-kernel-architecture
description: Apply when designing or modifying Bharat-OS kernel, HAL, architecture, driver, service, capability, scheduler, memory, IPC, or ownership behavior.
---

# Bharat-OS Kernel Architecture Skill

## Goal

Preserve a small capability microkernel while moving implementation toward per-core and distributed ownership.

## Procedure

1. Read root `AGENTS.md`, relevant architecture documents, and accepted ADRs.
2. Classify each changed responsibility as kernel mechanism, service policy, driver hardware control, runtime/library logic, or composed stack.
3. Identify every mutable object's owner and lifecycle.
4. Identify capability checks at every external or cross-domain entry.
5. Identify all architecture and memory-protection backends affected: MMU, MMU-Lite, MPU.
6. Define failure behavior: rollback, retry, reject, poison/quarantine, or explicit unsupported.
7. Reject designs using global mutable shortcuts, direct remote writes, pointer-bearing messages, unbounded spin loops, or hidden backend bypasses.
8. Add tests and update the architecture/contract documentation.

## Required design notes

Before implementation, record:

- object ownership,
- legal state transitions,
- synchronization/lock ordering,
- cross-core message schema,
- deadline/retry/idempotence behavior,
- capability object/rights/scope checks,
- backend behavior and fail-closed result,
- compensation after partial failure.

## Placement rule

- Kernel: deterministic mechanisms only.
- Services: policy, orchestration, routing, fallback, model selection, lifecycle supervision.
- Drivers: hardware control.
- Runtime/lib: backend/model/graph logic.
- `arch`: ISA implementation.
- `hal`: architecture-neutral contracts.
- Platform: machine/SoC/board wiring and discovery.
