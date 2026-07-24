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
    G->>G: validate context, flags, policy (traits + caps)
    G->>H: handler(ctx)
    H->>K: typed operation / service call
    H-->>G: result
    G-->>P: status (normalized long)
    P-->>T: return to trap_dispatch_syscall
    T->>A: arch_trap_set_syscall_return(frame, rc)
    T-->>U: return to user
```

### B. Component Ownership

```mermaid
flowchart TD
    Arch[arch/* syscall_extract.c] --> Gate[core/kernel/src/trap/syscall_gate.c]
    Trap[core/kernel/src/trap/syscall.c] --> Pers[personality_ops.handle_syscall]
    Pers --> Gate
    Gate --> Native[core/personalities/native/native_syscall.c]
    Gate --> Linux[core/personalities/compat/linux/linux_syscall.c]
    Gate --> Android[core/personalities/compat/android/android_syscall.c]
    Gate --> Windows[core/personalities/compat/windows/windows_syscall.c]

    UAPI[interface/include/bharat/uapi/syscall] --> Native
    UAPI --> SDK[experience/user/sdk]
```

### C. Implementation Status

- **Common Syscall Gate:** Complete. Enforces profile traits and capability rights.
- **Return Register Ownership:** Always in `trap_dispatch_syscall()`.
- **Usercopy:** Stage 2A (VMA-backed) validation complete.
- **x86_64 Fast Path:** Experimental (Disabled by default). Uses `int $0x80` transitional path in production.
- **Personalities:** Native and Linux (Hardened). Android and Windows (Scaffolds).

## Profile-Aware Policy (Traits)

Syscall enforcement depends on **traits** defined in the active profile policy, not on hardcoded profile enums.

| Trait | Impact |
| :--- | :--- |
| `BH_PROFILE_TRAIT_SERVICE_RICH` | Allows `BH_SYSCALL_F_SERVICE_CALL` syscalls. |
| `BH_PROFILE_TRAIT_NO_BLOCKING_RT` | Denies `BH_SYSCALL_F_BLOCKING` syscalls for RT threads. |
| `BH_PROFILE_TRAIT_MMU_FULL` | Requires VMA-backed Stage 2A usercopy validation. |

## Usercopy Hardening (Stage 2A)

All data transfers between kernel and userspace use checked primitives that validate against the process address space (VMA/Regions).

**Hardening Rules:**
- NULL rejection if `len > 0`.
- Pointer + length overflow checks.
- Strict max copy size: `BH_USERCOPY_MAX_BYTES` (4096).
- Authoritative VMA/Region validation (Permissions + Mapping).

## Architecture Notes

### x86_64
The x86_64 ABI is currently using `int $0x80` for stability. `SYSCALL/SYSRET` is experimental and requires `BHARAT_X86_64_ENABLE_FAST_SYSCALL`.

### ARM64
ARM64 syscall detection decodes `ESR_EL1.EC` (0x15).
