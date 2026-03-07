# Syscall and Trap Gate Baseline (v1)

This document defines the current in-kernel syscall/trap gate contract.

## Implemented baseline

- Central trap frame model (`trap_frame_t`) with register argument ABI slots.
- Syscall number space (`syscall_id_t`) for privileged kernel operations:
  - thread create/destroy,
  - scheduler yield,
  - VMM map/unmap,
  - capability invoke hook,
  - endpoint create/send/receive,
  - capability delegation.
- Dispatcher (`syscall_dispatch`) with per-call parameter validation.
- Trap handler (`trap_handle`) with:
  - cause filtering,
  - user-mode privilege check,
  - return code propagation to ABI return register,
  - architecture-aware PC advance after syscall.
- Boot-time trap gate setup via `trap_init()` from `kernel_main`.

## Security baseline checks

- Reject non-user trap-originated syscall attempts.
- Reject unsupported syscall IDs with explicit `-ENOSYS`-style code.
- Validate user pointer range before writing syscall outputs.

## Deferred for production

- Assembly entry stubs per architecture (x86_64 IDT/syscall entry, RISC-V `stvec`).
- Fine-grained capability policy enforcement in `cap_invoke` path.
- Full trap delegation verification and interrupt controller integration.
