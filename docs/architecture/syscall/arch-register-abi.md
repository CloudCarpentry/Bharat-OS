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

| Syscall | Register | trap_frame_t.gpr | Notes |
| :--- | :--- | :--- | :--- |
| nr | `x8` | `gpr[8]` | |
| arg0 | `x0` | `gpr[0]` | |
| arg1 | `x1` | `gpr[1]` | |
| arg2 | `x2` | `gpr[2]` | |
| arg3 | `x3` | `gpr[3]` | |
| arg4 | `x4` | `gpr[4]` | |
| arg5 | `x5` | `gpr[5]` | |
| return | `x0` | `gpr[0]` | |

## RISC-V 64-bit

| Syscall | Register | trap_frame_t.gpr | Notes |
| :--- | :--- | :--- | :--- |
| nr | `a7` | `gpr[17]` | |
| arg0 | `a0` | `gpr[10]` | |
| arg1 | `a1` | `gpr[11]` | |
| arg2 | `a2` | `gpr[12]` | |
| arg3 | `a3` | `gpr[13]` | |
| arg4 | `a4` | `gpr[14]` | |
| arg5 | `a5` | `gpr[15]` | |
| return | `a0` | `gpr[10]` | |

## Verification

Register mappings are verified using host-based unit tests in `quality/tests/host/syscall/`. These tests ensure that the extraction logic remains consistent with the `trap_frame_t` layout.
