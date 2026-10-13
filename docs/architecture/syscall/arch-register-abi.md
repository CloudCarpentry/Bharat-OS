---
title: Bharat-OS Architecture Register ABI
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
# Bharat-OS Architecture Register ABI

This document defines the register mapping for syscall extraction across all supported architectures.

## x86_64

| Syscall | Register | trap_frame_t.gpr | Notes |
| :--- | :--- | :--- | :--- |
| nr | `rax` | `gpr[0]` | |
| arg0 | `rdi` | `gpr[1]` | |
| arg1 | `rsi` | `gpr[2]` | |
| arg2 | `rdx` | `gpr[3]` | |
| arg3 | `r10` | `gpr[4]` | `SYSCALL` convention (moves from `rcx`) |
| arg4 | `r8` | `gpr[5]` | |
| arg5 | `r9` | `gpr[6]` | |
| return | `rax` | `gpr[0]` | |

## ARM64

- **Instruction:** `svc #0`
- **Exception Class:** `ARM64_EC_SVC64` (`0x15`)

| Syscall | Register | trap_frame_t.gpr | Notes |
| :--- | :--- | :--- | :--- |
| nr | `x8` | `gpr[8]` | Architectural standard Linux/POSIX EABI |
| arg0 | `x0` | `gpr[0]` | |
| arg1 | `x1` | `gpr[1]` | |
| arg2 | `x2` | `gpr[2]` | |
| arg3 | `x3` | `gpr[3]` | |
| arg4 | `x4` | `gpr[4]` | |
| arg5 | `x5` | `gpr[5]` | |
| return | `x0` | `gpr[0]` | |

## ARM32

- **Instruction:** `svc 0`
- **Exception Vector:** `0x08` (SVC synchronous vector)

| Syscall | Register | trap_frame_t.gpr | Notes |
| :--- | :--- | :--- | :--- |
| nr | `r7` | `gpr[7]` | Architectural standard ARM EABI |
| arg0 | `r0` | `gpr[0]` | |
| arg1 | `r1` | `gpr[1]` | |
| arg2 | `r2` | `gpr[2]` | |
| arg3 | `r3` | `gpr[3]` | |
| arg4 | `r4` | `gpr[4]` | |
| arg5 | `r5` | `gpr[5]` | |
| return | `r0` | `gpr[0]` | |

## RISC-V 64-bit

- **Instruction:** `ecall`
- **Cause:** `8` (Environment call from U-mode)

| Syscall | Register | trap_frame_t.gpr | Notes |
| :--- | :--- | :--- | :--- |
| nr | `a7` (`x17`) | `gpr[16]` | `trap_entry.S` maps `x1` to `gpr[0]`, so `xN` is `gpr[N-1]` |
| arg0 | `a0` (`x10`) | `gpr[9]` | |
| arg1 | `a1` (`x11`) | `gpr[10]` | |
| arg2 | `a2` (`x12`) | `gpr[11]` | |
| arg3 | `a3` (`x13`) | `gpr[12]` | |
| arg4 | `a4` (`x14`) | `gpr[13]` | |
| arg5 | `a5` (`x15`) | `gpr[14]` | |
| return | `a0` (`x10`) | `gpr[9]` | |

## RISC-V 32-bit

- **Instruction:** `ecall`
- **Cause:** `8` (Environment call from U-mode)

| Syscall | Register | trap_frame_t.gpr | Notes |
| :--- | :--- | :--- | :--- |
| nr | `a7` (`x17`) | `gpr[16]` | `trap_entry.S` maps `x1` to `gpr[0]`, so `xN` is `gpr[N-1]` |
| arg0 | `a0` (`x10`) | `gpr[9]` | |
| arg1 | `a1` (`x11`) | `gpr[10]` | |
| arg2 | `a2` (`x12`) | `gpr[11]` | |
| arg3 | `a3` (`x13`) | `gpr[12]` | |
| arg4 | `a4` (`x14`) | `gpr[13]` | |
| arg5 | `a5` (`x15`) | `gpr[14]` | |
| return | `a0` (`x10`) | `gpr[9]` | |

## Unified Personality Syscall Normalization Flow

```
[User-space App (Linux / Android)]
       │
       ▼ (Architecture instruction: syscall / svc / ecall)
[CPU Trap Vector]
       │
       ▼ (arch_trap_extract_syscall)
[bh_syscall_regs_t (nr, arg[0..5])]
       │
       ▼ (bh_syscall_gate / personality table lookup)
[Linux / Android Syscall Handler (bh_syscall_meta_t)]
       │
       ▼ (Return bh_operation_result_t)
[Status Conversion: linux_errno_from_bh_status (-errno)]
       │
       ▼ (arch_trap_set_syscall_return)
[Return to User Mode: rax / x0 / r0 / a0]
```

## Verification

Register mappings are verified across all 5 architectures in the mandatory QEMU matrix and host-based unit tests (`quality/tests/host/syscall/`). These tests ensure that the extraction logic remains consistent with the `trap_frame_t` layout.
