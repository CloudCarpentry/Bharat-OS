---
title: Syscall ABI Boundary Hardening
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - syscall
see_also:
  - README.md
---
# Syscall ABI Boundary Hardening

This document explains the architecture and governance of the Bharat-OS native syscall boundary.

## Native Syscall Dispatch Flow

1.  **Trap Entry:** When a userspace process executes a syscall instruction (`ecall`, `int 0x80`, etc.), the hardware traps into the kernel.
2.  **Architecture Trap Handler:** The `core/arch/` layer extracts the trap frame and identifies the trap class as `TRAP_CLASS_SYSCALL`.
3.  **Personality Dispatch:** The generic `trap_handle` calls `trap_dispatch_syscall`, which delegates to the `personality_ops->handle_syscall` of the current process.
4.  **Native Handler:** For native processes, this resolves to `default_handle_syscall` (in `core/personalities/native/personality_native.c`), which calls `syscall_dispatch`.
5.  **Execution:** `syscall_dispatch` (in `core/kernel/src/trap/trap.c`) performs parameter validation and invokes the internal kernel service.
6.  **Return Mapping:** The internal `kstatus_t` result is mapped to a `sys_errno_t` via `kstatus_to_sysret` before returning to userspace.

## kstatus_t vs sys_errno_t

Bharat-OS maintains a strict separation between internal kernel status and the external syscall ABI:

-   **`kstatus_t`:** Rich, internal kernel status codes (defined in `core/kernel/include/core/kernel/status.h`). Used for precise error reporting between kernel subsystems.
-   **`sys_errno_t`:** Stable, POSIX-style error codes returned to userspace (defined in `interface/include/bharat/interface/uapi/sys_errno.h`).

**Rule:** Kernel code must never return raw `sys_errno_t` values. All internal functions return `kstatus_t`. Translation occurs exclusively at the syscall boundary using `kstatus_to_sysret()`.

## Syscall ABI Governance

To ensure long-term stability and compatibility, the following rules apply to `interface/include/bharat/interface/uapi/syscall_table.def`:

1.  **Append-Only:** New syscalls must be added to the end of the table with a new unique number.
2.  **No Renumbering:** Once assigned, a syscall number must never change.
3.  **No Deletions:** Syscalls cannot be removed from the table. If a syscall is deprecated, it should be kept as a stub returning `K_ERR_UNSUPPORTED`.
4.  **No Renames:** Syscall names in the UAPI are part of the contract and should not be changed.

### ABI Drift Verification

The `tools/abi/check_syscalls.py` tool enforces these rules by comparing the current `syscall_table.def` against a committed baseline manifest in `interface/contracts/abi/syscalls.json`. This check is part of the CI pipeline.

## Syscall Implementation Backends

To maintain a clean ABI boundary, Bharat-OS distinguishes between architecture-specific syscall entries and generic fallbacks:

-   **Arch-specific Assembly:** (e.g., `experience/user/sdk/lib/src/riscv_syscalls.S`). This is the preferred implementation for production-grade SDK and user-space binaries. It provides the thinnest possible wrapper around the hardware trap instruction.
-   **Generic Fallback:** (`core/lib/syscall/syscall_stubs.c`). This implementation provides a C-based fallback and is primarily intended for host-based testing or early bring-up of new architectures.

**Rule:** `core/lib/syscall/syscall_stubs.c` must not be linked when an architecture-specific syscall backend is present. This is enforced by guarding the `bharat_syscall` definition in `syscall_stubs.c` with the `BHARAT_HAS_ARCH_SYSCALL_BACKEND` macro. Architecture-specific assembly backends are located in the `experience/user/sdk/lib/src/` directory and integrated via `core/lib/syscall/CMakeLists.txt`.

## Why Native ABI Hardening First?

Hardening the native syscall boundary and establishing ABI drift gates is a prerequisite for implementing Linux compatibility (personalities). A stable and secure native foundation ensures that compatibility layers are built on a well-defined contract, preventing "leaks" of internal kernel state and ensuring that security invariants are maintained across different execution personalities.
