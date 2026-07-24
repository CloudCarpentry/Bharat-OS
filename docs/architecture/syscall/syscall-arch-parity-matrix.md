# Bharat-OS Syscall Architecture Parity Matrix

This document tracks the status of syscall and userspace support across all supported hardware architectures.

| Arch | Bits | Kernel Trap Entry | Arch Syscall Extractor | SDK Syscall Backend | Userspace Supported | Default Status | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| x86_64 | 64 | Present | Present | Present | Yes | Production | SYSCALL/SYSRET production path. |
| arm64 | 64 | Present | Present | Present | Yes | Production | |
| riscv64 | 64 | Present | Present | Present | Yes | Production | |
| arm32 | 32 | Scaffold | None | None | No | Unsupported | Requires SVC trap recognition. |
| riscv32 | 32 | Scaffold | None | None | No | Unsupported | Requires ecall trap recognition. |

## Status Definitions

*   **Production**: Full support with hardened trap entry and SDK backends.
*   **Transitional**: Functional but using non-optimal or legacy paths (e.g., INT 0x80 on x86_64).
*   **Experimental**: Initial implementation behind non-default build flags.
*   **Compile-only**: Minimal code for kernel build, no userspace support.
*   **Unsupported**: No syscall/trap support implemented or explicitly disabled.

## Capability Macros

The build system uses the following macros to gate syscall-related code:

*   `BHARAT_ARCH_HAS_USER_SYSCALL_ENTRY`: Kernel has a valid trap path for userspace syscalls.
*   `BHARAT_ARCH_HAS_SYSCALL_EXTRACT`: Kernel has an architecture-specific extractor for syscall registers.
*   `BHARAT_USERSPACE_SUPPORTED`: Architecture is fully capable of running userspace applications with syscalls.
