# Bharat-OS Syscall ABI Contract

## Status
Accepted

## Scope
This document defines the stable ABI contract for syscalls in Bharat-OS, covering register conventions, return values, and metadata rules.

## Non-Goals
This does not cover specific handler implementations or higher-level library APIs.

## Contract
- Syscall numbers are defined in `bh_syscall_numbers.h` and are append-only.
- All syscalls must pass through the `bh_syscall_gate`.
- Max 6 arguments are supported.

## Per-Architecture Behavior
| Arch | Inst | Syscall Nr | Args | Return |
|------|------|------------|------|--------|
| x86_64 | syscall | rax | rdi, rsi, rdx, r10, r8, r9 | rax |
| arm64 | svc #0 | x8 | x0-x5 | x0 |
| arm32 | svc #0 | r7 | r0-r5 | r0 |
| riscv64| ecall | a7 | a0-a5 | a0 |
| riscv32| ecall | a7 | a0-a5 | a0 |

## Failure Behavior
- Invalid syscall numbers return `BH_ERR_INVALID_SYSCALL`.
- Capability failures return `BH_ERR_ACCESS_DENIED`.
- Usercopy failures return `BH_ERR_FAULT`.

## Security Invariants
- `BH_SYSCALL_F_CAP_REQUIRED` must be enforced by the gate.
- Fast syscalls must not block or perform usercopy.
- User pointers must be validated via `mm_user_range_validate_current`.

## Testing Requirements
- `quality/tests/host/syscall/` covers extractor and status mapping.
- QEMU smoke tests for each architecture.

## Migration Notes
Legacy `trap_user_ptr_valid` in `trap.c` is superseded by `mm_user_range_validate_current`.
