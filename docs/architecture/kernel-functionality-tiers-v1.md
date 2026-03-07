# Kernel Functionality Tiers (v1)

## Purpose

This document defines what Bharat-OS **must have**, **should have soon after**, and **explicitly defers** for v1.

The goal is to prevent scope drift and make progress measurable against the stated microkernel spine.

## Tier A — Required for the v1 bootable spine

The following items are **hard requirements** for claiming a v1 bootable kernel spine:

1. Boot entry and architecture handoff
2. Early serial console
3. Panic path
4. PMM (physical memory manager)
5. Kernel heap or early allocator
6. VMM scaffold
7. Interrupt/trap handling path
8. Timer source
9. Idle thread
10. Scheduler scaffold
11. Capability table basics
12. Endpoint/message IPC basics
13. Kernel self-test harness

If any Tier A item is missing, v1 is not complete.

## Tier B — Required before calling it a microkernel prototype

These items are the next bar after Tier A and should be implemented before calling Bharat-OS a functional microkernel prototype:

1. User task creation
2. Address-space object abstraction
3. Isolated user-space service
4. Driver boundary model stub
5. Syscall/trap gate contract
6. Basic fault isolation behavior
7. Structured boot log/event codes

Tier B defines the minimum transition from "bootable kernel scaffold" to "microkernel prototype".

## Tier C — Explicitly deferred (out of v1 core)

The following are explicitly **deferred** and should not block v1 completion:

- GUI compositor
- Desktop shell
- Network stack
- Filesystem richness
- AI governor
- Advanced personality layers
- Distributed/fabric features
- arm64 runtime support (arm64 remains compile-validation in v1)

## Enforcement and reporting

- CI should continuously validate Tier A runtime evidence on x86_64.
- riscv64 should remain compile-validated and be promoted to stronger runtime checks as stability improves.
- Release notes and status updates should report progress by Tier (A/B/C), not by broad marketing labels.

## Relationship to other docs

- Boot success and serial evidence definitions: [`v1-boot-definition.md`](v1-boot-definition.md)
- Architecture index and scope notes: [`README.md`](README.md)
- Final conflict resolution on scope/priority: ADRs under `docs/decisions/`
