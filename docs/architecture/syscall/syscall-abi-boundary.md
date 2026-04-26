---
title: Syscall ABI Boundary Hardening
status: Active
owner: Documentation Working Group
last_updated: 2026-06-05
tags:
  - docs
  - architecture
  - syscall
see_also:
  - README.md
---
# Syscall ABI Boundary Hardening

This document explains the architecture and governance of the Bharat-OS secure syscall substrate.

## Common Syscall Substrate Flow

Bharat-OS uses a unified, high-performance secure substrate for all syscall personalities (Native, Linux, Android, Windows).

### A. Unified Syscall Flow

```mermaid
sequenceDiagram
    participant U as User code / SDK / Compat app
    participant A as Arch syscall entry
    participant T as trap_dispatch_syscall
    participant P as Personality ops
    participant G as bh_syscall_gate
    participant D as Personality syscall table
    participant H as Syscall handler
    participant K as Kernel / Service primitive

    U->>A: syscall / svc / ecall / int 0x80
    A->>T: trap_frame + trap_info
    T->>P: handle_syscall(frame, info)
    P->>G: common gate
    G->>A: arch_trap_extract_syscall()
    G->>D: lookup descriptor by personality + nr
    G->>G: validate context, flags, policy
    G->>H: handler(ctx)
    H->>K: typed operation / service call
    H-->>G: result
    G->>A: arch_trap_set_syscall_return()
    G-->>P: status
    P-->>T: return to user
```

### B. Component Ownership

```mermaid
flowchart TD
    Arch[arch/* syscall_extract.c] --> Gate[core/kernel/src/trap/syscall_gate.c]
    Trap[core/kernel/src/trap/syscall.c] --> Pers[personality_ops.handle_syscall]
    Pers --> Gate
    Gate --> Native[core/personalities/native/native_syscall.c]
    Gate --> Linux[core/personalities/compat/linux/linux_syscall.c]
    Gate --> Android[future android overlay]
    Gate --> Windows[future NT-style personality]

    UAPI[interface/include/bharat/uapi/syscall] --> Native
    UAPI --> SDK[experience/user/sdk]
```

### C. Implementation Status

```mermaid
flowchart LR
    Done[Complete] --> Gate[Common syscall gate]
    Done --> Native[Native table]
    Done --> LinuxMin[Minimal Linux table]
    Done --> Usercopy[Stage 1.5 usercopy]
    Done --> X86Int[x86_64 int 0x80 transitional]
    Future[Future] --> X86Fast[x86_64 syscall/sysret]
    Future --> VMA[VMA/page-backed usercopy]
    Future --> Android[Android overlay]
    Future --> Windows[Windows NT object model]
    Future --> VDSO[vDSO fast page]
```

## Governance and ABI

The canonical source of truth for syscall numbers is `interface/include/bharat/uapi/syscall/table.def` and the manifest `interface/contracts/abi/syscalls.json`.

1.  **Append-Only:** New syscalls must be added to the end of the table.
2.  **No Renumbering:** Syscall numbers are immutable once assigned.
3.  **No Deletions:** Deprecated syscalls become stubs returning `K_ERR_UNSUPPORTED`.
4.  **No Renames:** UAPI names are part of the stable contract.

### ABI Drift Verification

The `tools/abi/check_syscalls.py` tool enforces these rules against the baseline manifest.

## Usercopy Hardening (Stage 1.5)

All data transfers between kernel and userspace must use the checked usercopy primitives:

-   `copy_from_user_checked()`
-   `copy_to_user_checked()`
-   `copy_user_string_checked()`

**Hardening Rules:**
- NULL rejection if `len > 0`.
- Pointer + length overflow checks.
- Strict max copy size: `BH_USERCOPY_MAX_BYTES` (default 4096).
- User range validation.

## Architecture Notes

### x86_64
The x86_64 ABI is temporarily using `int $0x80` for consistency. The target production ABI is `SYSCALL/SYSRET`.

### ARM64
ARM64 syscall detection decodes `ESR_EL1.EC` (0x15).
```c
/* Bharat-OS currently treats all SVC-from-EL0 exceptions as syscall entry.
 * ISS/SVC immediate is reserved for future ABI versioning/debug use.
 */
```
