# Architecture Documentation Index

This folder captures the current architectural baseline for Bharat-OS.

## How to read this section

1. Start with platform bring-up flows.
2. Continue with kernel security and object/IPC/memory models.
3. Review deferred areas (GUI, personalities) with their status labels.
4. Cross-check major architectural constraints against ADRs in `docs/decisions/`.

## Recommended reading order

### Boot and bring-up

- [`boot-flow-x86_64.md`](boot-flow-x86_64.md)
- [`boot-flow-riscv64.md`](boot-flow-riscv64.md)

### Core kernel architecture

- [`capability-model.md`](capability-model.md)
- [`kernel-object-model.md`](kernel-object-model.md)
- [`ipc-model.md`](ipc-model.md)
- [`memory-model.md`](memory-model.md)
- [`driver-model.md`](driver-model.md)
- [`storage-and-sandbox-strategy.md`](storage-and-sandbox-strategy.md)
- [`verification-scope.md`](verification-scope.md)

### User-space strategy and deferred surfaces

- [`personality-layer.md`](personality-layer.md)
- [`gui-strategy.md`](gui-strategy.md)

## Scope note

Architecture docs describe both:

- **In-scope v1 core** (bootable, verifiable kernel spine), and
- **Deferred/experimental directions** (documented for design continuity).

The ADR set is the source of truth whenever scope or priority conflicts arise.
