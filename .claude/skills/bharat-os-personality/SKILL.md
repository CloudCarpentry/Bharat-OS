---
name: bharat-os-personality
description: Apply when working on Linux, Android, or POSIX compatibility personalities, syscall translation, Binder shims, futex emulation, or VFS adaptation.
---

# Bharat-OS Personality Skill

## Mission

Bharat-OS supports ecosystem personalities (Linux, Android, POSIX) to allow running unmodified applications and libraries on top of its capability microkernel without polluting the trusted kernel computing base.

## Architectural Boundaries

1. **Isolation from Kernel Core**:
   - Compatibility personalities live strictly in `core/personalities/compat/` or in userland compatibility services.
   - **Never place Linux or Android specific data structures, syscall numbers, or emulation hooks inside `core/kernel/`.**
   - The kernel core exposes only clean, native Bharat-OS capabilities and syscalls.
2. **Translation Layer Mechanics**:
   - Personalities act as translation shims that map foreign interfaces to native Bharat-OS primitives:
     - Linux `futex` -> Native kernel waitqueue / fast-lock mechanism.
     - Linux `epoll` / `eventfd` / `timerfd` -> Native async notification channels and HAL timers.
     - Linux `fork` / `execve` -> Native `process_manager` lifecycle + `vm_manager` address space creation.
     - Android `Binder` -> Native microkernel IPC / uRPC capability-delegated endpoints.
     - Linux `procfs` / `sysfs` -> Virtual filesystem service routing.
3. **Security Invariants**:
   - Foreign syscall translation must enforce the same capability mediation as native operations.
   - A compromised or misbehaving personality layer cannot elevate privileges; it is bound by the capabilities granted to its process.
4. **Maturity & Partial Compatibility Reporting**:
   - Any personality feature that is scaffolded or only partially implemented must be explicitly recorded in `interface/contracts/implementation_maturity.json`.
   - Never claim full Linux or Android binary compatibility until proven by end-to-end userland test suites.

## Verification Checklist

- [ ] Ensure no personality includes or definitions leak into `core/kernel/` (run `python3 tools/lint/check_layer_references.py`).
- [ ] Run focused personality unit tests (e.g. `quality/tests/personality/`).
- [ ] Verify that foreign syscall numbers are translated in the personality layer and not registered in `native_syscalls.json`.
- [ ] Test error conversion: ensure foreign errno values (e.g. `EINVAL`, `EFAULT`, `EPERM`) map deterministically to native `BH_STATUS_*` codes and vice-versa.
