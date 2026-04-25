---
title: Capability Model
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# Capability Model

**See also:**
- [Capability Naming and Terms](capability-naming-and-terms.md)
- [Distributed Capability Consistency](distributed-capability-consistency.md)
- [Naming Conventions](../../contracts/naming-conventions.md)

## Overview

Bharat-OS enforces security through a mathematically verifiable Capability System. There are no global Access Control Lists (ACLs), user IDs, or root privileges inside the kernel.

## The Golden Rule: "Never Trust, Always Verify"

A capability is an unforgeable, kernel-managed token that pairs an object reference with a set of permitted operations (Read, Write, Execute, Grant). If a thread does not hold a capability to an object in its Capability Space (CSpace), the object does not functionally exist to that thread.

## Capability Operations

- **Invoke**: Perform an action on the object the capability points to (e.g., mapping a memory frame into an address space).
- **Grant**: Transfer a capability over an IPC Endpoint to another task. IPC capability transfer involves a strict policy validation (transferability of type, rights attenuation) and safely delegates the capability into the receiver's CSpace.
- **Revoke**: Recursively invalidate a capability and all derivations of it. This spans across cross-process boundaries to safely sever derived authorities.
- **Retype**: Convert an `Untyped` memory block capability into specific kernel objects (Threads, Endpoints, Frames).

## Security Benefits

- **Zero-Trust**: No ambient authority.
- **Verifiability**: The flow of authority can be graph-analyzed at compile time to mathematically prove isolation between critical domains (e.g., separating networking stacks from AI cryptographic enclaves).

> Reference: seL4 capability systems define a capability as an immutable reference paired with rights, where capability invocation is the authority path that enables least-privilege control.

## Capability and IPC Baseline (Capabilities Portion)

- Per-process capability table (`capability_table_t`) allocated during `process_create`.
- Capability entries include:
  - object type,
  - object reference,
  - fine-grained rights (`SEND`, `RECEIVE`, `MAP`, `UNMAP`, `SCHEDULE`, `DELEGATE`).
- Capability transfer policies: A strict set of rules evaluating transferable types and transferable rights attenuation. Only explicit capabilities carrying `DELEGATE` right can be transferred across domains.
- Delegation support enforces right reduction (`cap_table_delegate` requires delegated rights to be a subset of source rights).
- Revocation paths follow tree structures to gracefully revoke derived and explicitly granted capabilities recursively.