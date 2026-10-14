---
name: bharat-os-kernel-architecture
description: Apply when designing or modifying Bharat-OS kernel, HAL, architecture, driver, service, capability, scheduler, memory, IPC, or ownership behavior.
---

# Bharat-OS Kernel Architecture Skill

## Goal

Preserve a minimal, verification-friendly capability microkernel while moving implementation systematically toward per-core and distributed ownership.

## Placement & Layering Rules

Bharat-OS enforces strict boundary separation:

- **`core/kernel/`**: Deterministic mechanisms only (scheduling primitives, memory mapping mechanisms, capability translation, trap/syscall entry, IPC/uRPC delivery, minimal synchronization). **No policy, no drivers, no orchestration.**
- **`core/services/`**: System policy, resource management, process lifecycle, virtual memory authority, driver orchestration, devmgr, networking routing, supervisor health monitoring.
- **`core/drivers/`**: Hardware control and driver mechanisms. No services and no kernel policy.
- **`core/stacks/`**: Composed protocol/domain stacks (network, storage, USB).
- **`core/lib/`**: Reusable standalone algorithms/utilities without kernel policy.
- **`core/arch/`**: ISA-specific instructions, CPU registers, trap vectors, context save/restore.
- **`core/hal/`**: Architecture-neutral contracts, timer/interrupt interfaces, MMU/MPU contract glue.
- **`platform/`**: Machine/SoC topology, device tree discovery, board wiring.

## Architectural Invariants

1. **Mechanism vs. Policy**: The kernel enforces *how* a resource is mediated; userland services decide *which* process gets the resource and *when*.
2. **Capability-Mediated**: Every resource access without exception must be guarded by an unforgeable capability token. Deny by default, fail closed.
3. **Core-Local Ownership**: Mutable state belongs to a single core. Never use global shared mutable variables as a shortcut to avoid ownership design.
4. **Backend Neutrality**: Code must not assume a full MMU exists unless gated by `MMU_FULL`. Support MMU-Lite and MPU/PMP or return explicit `BH_STATUS_ERR_UNSUPPORTED`.
5. **No Direct Remote Mutation**: Cross-core coordination occurs solely via message protocols, never by reaching into another core's private runqueue or memory structures.
6. **Hardware Discovery**: Dynamic hardware facts (memory size, core count, peripherals) are discovered via HAL/platform/device-tree, never hardcoded in compile-time configuration.

## Required Design Record

Before implementing an architectural change, document:
- Object ownership and lifecycle states (`ACTIVE`, `MIGRATING`, `DYING`, `DEAD`).
- Synchronization model, lock ordering, and bounded lock hold times.
- Cross-domain message structures (fixed-width, no raw pointers).
- Capability type, rights, scope, and generation checks.
- Backend implications (MMU_FULL, MMU_LITE, MPU/PMP).
- Safe rollback or object quarantine in the event of partial failure.
