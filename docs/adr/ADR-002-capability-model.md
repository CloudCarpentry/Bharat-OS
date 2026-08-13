---
title: "ADR-002: The Capability Security Model"
status: Accepted
owner: Documentation Working Group
last_updated: 2026-08-13
tags:
  - docs
  - adr
see_also:
  - README.md
---
# ADR-002: The Capability Security Model

## Status

Accepted

## Context

Traditional operating systems rely on Access Control Lists (ACLs) checking User IDs (UIDs) against file permissions. This model suffers from the "Confused Deputy Problem," where a privileged software component can be tricked into misusing its authority on behalf of an attacker.

## Decision

We adopted a pure **Capability-Based Object Model**.
A capability is an unforgeable token (managed entirely in kernel memory so user-space cannot tamper with it) that acts as both a pointer to an object and a set of permission rights (Read/Write/Grant).

```mermaid
flowchart TD
    subgraph Userspace["User Space Process (CSpace)"]
        Cap1[Capability A (Read/Write)]
        Cap2[Capability B (Grant)]
    end

    subgraph Kernel["Microkernel Memory"]
        Obj1[(Kernel Object X)]
        Obj2[(Kernel Object Y)]
    end

    Cap1 -->|Verified Reference| Obj1
    Cap2 -->|Verified Reference| Obj2

    %% Tampering attempt fails
    Tamper[Malicious Code] -.->|Invalid direct memory access| Obj1
    style Tamper fill:#f9f,stroke:#333,stroke-width:2px
```

If a thread does not possess a capability (in its Capability Space, or CSpace) pointing to an object, that object does not functionally exist to the thread.

## Consequences

### Process-owned CSpaces and core ownership

The process owns its CSpace object. A CPU is only the temporary mutation owner;
CPU identity is not CSpace identity and does not determine CSpace capacity.
Kernel storage is allocated per process and registered in a bounded owner-core
registry. Fixed-width locators name the CSpace, owner core, slot generation, and
revocation epoch without carrying a table pointer.

The registry is owner-local mutable state. Cross-core mutation uses bounded uRPC
requests, and destruction unpublishes the registry entry before freeing the
table so delayed locators fail closed. The current implementation does not yet
transfer CSpace mutation ownership between cores; such a transfer requires an
acknowledged, generation-changing transaction with rollback rather than direct
remote mutation.

### Positive

- **Zero-Trust Enforcement**: No ambient authority or "root user" exists. Every action must be explicitly authorized.
- **Delegation & Confinement**: A task can securely delegate a stripped-down version of its own capability to a restricted sandboxed process, proving mathematical confinement.
- **Deterministic Resource Tracking**: Memory accounting is flawless because memory leaks are impossible; when a capability tree is revoked, all derived memory is recursively freed.

### Negative

- **Programming Complexity**: Writing POSIX applications natively is difficult because "opening a file by string path" doesn't map cleanly to passing capability tokens. We mitigate this by establishing POSIX-Native translation daemons in user-space.
