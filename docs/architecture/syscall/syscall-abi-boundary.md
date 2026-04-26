---
title: Syscall ABI Boundary Hardening
status: Active
owner: Documentation Working Group
last_updated: 2026-05-22
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

1.  **Trap Entry:** Userspace executes a syscall instruction (`SVC`, `ecall`, `int 0x80`).
2.  **Architecture Trap Handler:** `core/arch/` extracts the trap frame and classifies it using `arch_trap_is_syscall(frame)`.
3.  **Trap Dispatch:** `trap_handle` calls `trap_dispatch_syscall(frame, info)`.
4.  **Personality Routing:** `trap_dispatch_syscall` (in `core/kernel/src/trap/syscall.c`) routes to the current thread's `personality_ops->handle_syscall`.
5.  **Common Syscall Gate:** The personality handler (e.g., Native or Linux) calls the common `bh_syscall_gate(frame, info)`.
6.  **Secure Substrate (`bh_syscall_gate`):**
    -   Calls `arch_trap_extract_syscall()` to fill `bh_syscall_regs_t`.
    -   Identifies personality and looks up the associated `bh_personality_syscall_table_t`.
    -   Validates syscall number bounds and looks up the `bh_syscall_desc_t`.
    -   Updates performance observability stats (total, fast, slow).
    -   Executes the typed handler.
7.  **Return Path:** `trap_dispatch_syscall` calls `arch_trap_set_syscall_return()` to place the result in the correct architectural register.

## kstatus_t vs sys_errno_t

Bharat-OS maintains a strict separation between internal kernel status and the external syscall ABI:

-   **`kstatus_t`:** Internal kernel status codes (defined in `core/kernel/include/core/kernel/status.h`).
-   **`sys_errno_t`:** Stable error codes returned to userspace (defined in `interface/include/bharat/uapi/sys_errno.h`).

**Translation Rule:** Translation occurs via `kstatus_to_sysret()` in the syscall dispatch path or within the personality-specific error normalization logic.

## Syscall ABI Governance

The canonical source of truth for syscall numbers is `interface/include/bharat/uapi/syscall/table.def` and the manifest `interface/contracts/abi/syscalls.json`.

1.  **Append-Only:** New syscalls must be added to the end of the table.
2.  **No Renumbering:** Syscall numbers are immutable once assigned.
3.  **No Deletions:** Deprecated syscalls become stubs returning `K_ERR_UNSUPPORTED`.
4.  **No Renames:** UAPI names are part of the stable contract.

### ABI Drift Verification

The `tools/abi/check_syscalls.py` tool enforces these rules against the baseline manifest.

## Syscall Implementation Backends

-   **Arch-specific Assembly:** (e.g., `experience/user/sdk/lib/src/x86_64_syscalls.S`). Preferred for production.
-   **Generic Fallback:** (`core/lib/syscall/syscall_stubs.c`). For host-tests or bring-up.

### x86_64 Architecture Note
The x86_64 ABI is temporarily using `int $0x80` for consistency between SDK and kernel. The target production ABI is `SYSCALL/SYSRET`. Do not benchmark performance on the transitional path.

### ARM64 Architecture Note
ARM64 syscall detection decodes `ESR_EL1.EC` (bits [31:26]) to identify `SVC` from lower Exception Levels (EC 0x15).

## Usercopy Hardening

All data transfers between kernel and userspace must use the checked usercopy primitives:

-   `copy_from_user_checked()`
-   `copy_to_user_checked()`
-   `copy_user_string_checked()`

Current hardening (Stage 1.5) includes NULL rejection, pointer overflow checks, and max copy size limits. Future Stage 2 will integrate with the VMM for full VMA/page permission validation.
