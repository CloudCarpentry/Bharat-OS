# x86_64 Syscall Entry Contract

This document describes the production syscall entry path for x86_64 in Bharat-OS.

## Overview

Bharat-OS uses the `SYSCALL/SYSRET` instructions for high-performance syscall entry on x86_64. The legacy `INT 0x80` path is retained only for transitional compatibility.

## Register ABI

| Purpose | User Register | Entry State | Notes |
| :--- | :--- | :--- | :--- |
| Syscall Number | `rax` | Preserved | |
| Argument 0 | `rdi` | Preserved | |
| Argument 1 | `rsi` | Preserved | |
| Argument 2 | `rdx` | Preserved | |
| Argument 3 | `r10` | Preserved | User `rcx` is moved to `r10` by convention (SYSCALL clobbers `rcx`) |
| Argument 4 | `r8` | Preserved | |
| Argument 5 | `r9` | Preserved | |
| Return Value | `rax` | Modified | |
| User RIP | `rcx` | Clobbered | Saved to `trap_frame_t.pc` |
| User RFLAGS | `r11` | Clobbered | Saved to `trap_frame_t.status` |

## Entry Procedure (`x86_64_syscall_entry`)

1.  **Swap GS**: Use `swapgs` to switch to the kernel's per-CPU `cpu_local_t` structure.
2.  **Stack Switch**: Save the user `%rsp` into `cpu_local_t.user_stack` and load the kernel stack from `cpu_local_t.kernel_stack`.
3.  **Frame Construction**: Build a `trap_frame_t` on the kernel stack.
    *   Save all General Purpose Registers.
    *   Store `rcx` as the user PC.
    *   Store `r11` as the user Status (RFLAGS).
    *   Store a pseudo-vector `0x100` as the trap cause.
4.  **Dispatch**: Call `bh_syscall_gate` with a pointer to the frame.
5.  **Restore**:
    *   Restore all GPRs (result in `rax`).
    *   Reload user `%rsp` from the frame.
6.  **Return**:
    *   `swapgs` back to user GS.
    *   `sysretq` to return to userspace.

## MSR Configuration

*   `STAR`: Configured with Kernel CS/SS (0x08/0x10) and User CS/SS (0x1B/0x23).
*   `LSTAR`: Points to `x86_64_syscall_entry`.
*   `SFMASK`: Masks `IF`, `TF`, `DF`, `AC`, `NT` bits in `RFLAGS`.
*   `EFER`: `SCE` bit enabled.

## Security Considerations

*   **Stack Isolation**: Every core has its own kernel stack to prevent cross-thread interference.
*   **GS Safety**: `swapgs` is used strictly on entry/exit to ensure kernel code always sees the correct `cpu_local_t`.
*   **Registers**: All registers are saved/restored to prevent information leaks, except those modified by the ABI.
