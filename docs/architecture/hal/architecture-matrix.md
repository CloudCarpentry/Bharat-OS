---
title: Architecture Matrix
status: Active
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - hal
see_also:
  - hal-tiers.md
---

# Architecture Matrix

This matrix represents the current verified state of architecture-specific implementations across the Bharat-OS tree, backed by CMake configuration and runtime evidence.

## Core Capability Matrix

| Feature | x86_64 | ARM64 | ARM32 | RISC-V64 | RISC-V32 |
|---|---|---|---|---|---|
| **Boot entry** | `Tier 3` | `Tier 3` | `Tier 2` | `Tier 3` | `Tier 2` |
| **Privilege mode** | Supported | Supported | Supported | Supported | Supported |
| **Trap frame** | Supported | Supported | Supported | Supported | Supported |
| **Syscall entry** | `HW_ENTRY` | `HW_ENTRY` | `HW_ENTRY` | `HW_ENTRY` | `HW_ENTRY` |
| **Context switch** | Supported | Supported | Supported | Supported | Supported |
| **Timer source** | Supported | Supported | Supported | Supported | Supported |
| **Interrupt controller**| Supported | Supported | Supported | Supported | Supported |
| **IPI/core notify** | Supported | Supported | Supported | Supported | Supported |
| **MMU/MPU model** | MMU-Full | MMU-Full | MMU-Lite / MPU | MMU-Full | MMU-Lite / MPU |
| **User-mode entry** | Supported | Supported | Supported | Supported | Supported |
| **SMP/AP bring-up** | `PARTIAL` | `PARTIAL` | `PARTIAL` | `PARTIAL` | `PARTIAL` |
| **TLB invalidation** | Supported | Supported | Supported | Supported | Supported |
| **Hardware memops** | Tier 0 | Tier 0 | Tier 0 | Tier 0 | Tier 0 |
| **Current runtime tier**| Tier 3 | Tier 3 | Tier 2 | Tier 3 | Tier 2 |

*Note: All architecture-specific syscall paths currently report `BHARAT_ARCH_HAS_COMPLETE_USERSPACE` via CMake for Tier 1 and Tier 2.*

## Flow Boundaries

Bharat-OS enforces strict separation between hardware details and kernel policy.

```mermaid
flowchart TD
  subgraph Kernel
    K[Kernel Policy & Scheduling]
  end
  subgraph HAL
    H[HAL Common Contracts\ncore/hal/]
  end
  subgraph Arch
    A[Architecture Backend\ncore/arch/<isa>/]
  end
  subgraph Platform
    P[Platform/SoC Integration\ncore/platform/]
  end

  K --> H
  H --> A
  H --> P
  A --> Hardware
  P --> Hardware
```

### Context Switch Flow (IMPLEMENTED)

```mermaid
sequenceDiagram
    participant S as Scheduler (core/kernel/src/sched)
    participant H as HAL Contract (core/hal/include/hal)
    participant A as Arch Code (core/arch/<isa>)

    S->>H: hal_cpu_context_switch(prev, next)
    H->>A: arch_context_switch()
    A->>A: Save general purpose registers
    A->>A: Restore next thread registers
    A-->>S: Return to next thread execution
```

### Syscall Flow (IMPLEMENTED)

```mermaid
sequenceDiagram
    participant U as Userspace App
    participant A as Arch Trap Entry (core/arch/<isa>)
    participant H as HAL Common Trap (core/hal)
    participant K as Kernel Syscall Dispatcher (core/kernel/src/syscall)

    U->>A: ecall / svc / syscall
    A->>A: Save userspace context to trap frame
    A->>H: hal_trap_dispatch()
    H->>K: syscall_handler()
    K->>K: Validate arguments & capability
    K->>H: Set return value in trap frame
    H->>A: hal_trap_return()
    A->>U: Restore context & return to user
```

### TLB Shootdown IPI Flow (IMPLEMENTED)

```mermaid
sequenceDiagram
    participant C1 as Core 0 (Initiator)
    participant H as HAL IPI (core/hal)
    participant C2 as Core 1 (Target)

    C1->>H: hal_tlb_shootdown_ipi(target_mask, aspace)
    H->>C2: Hardware Interrupt (IPI)
    C2->>C2: Invalidate local TLB
    C2->>H: IPI Ack
    H-->>C1: Synchronous completion
```
