# Bharat-OS Syscall Build Matrix

This document defines the supported syscall configurations across architecture families and bitness.

## Build Matrix

| Architecture | Bits | `BHARAT_ENABLE_USERSPACE` | `BHARAT_ARCH_HAS_USER_SYSCALL_ENTRY` | `BHARAT_ARCH_HAS_SYSCALL_EXTRACT` | `BHARAT_HAS_ARCH_SYSCALL_BACKEND` | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| x86_64 | 64 | Supported | Yes | Yes | Yes | |
| arm64 | 64 | Supported | Yes | Yes | Yes | |
| riscv64 | 64 | Supported | Yes | Yes | Yes | |
| arm32 | 32 | **NOT Supported** | No | No | No | Quarantined. |
| riscv32 | 32 | **NOT Supported** | No | No | No | Quarantined. |

## CMake Configuration Flags

The build system uses several flags to manage syscall support:

*   `BHARAT_ENABLE_USERSPACE`: Global flag to enable/disable userspace and syscall support.
*   `BHARAT_ARCH_HAS_USER_SYSCALL_ENTRY`: Indicates if the architecture has a valid trap entry for userspace syscalls.
*   `BHARAT_ARCH_HAS_SYSCALL_EXTRACT`: Indicates if the architecture has an implementation of `arch_trap_extract_syscall`.
*   `BHARAT_HAS_ARCH_SYSCALL_BACKEND`: Set by `core/lib/syscall/CMakeLists.txt` when a valid SDK assembly backend is selected.

## Consistency Checks

The following checks are performed at configure time:

1.  **Userspace Support**: If `BHARAT_ENABLE_USERSPACE` is `ON`, the architecture must be marked as supported. If not, `message(FATAL_ERROR)` is triggered.
2.  **SDK Backend**: If `BHARAT_ENABLE_USERSPACE` is `ON`, a valid architecture-specific assembly backend must be selected for the SDK.
3.  **Host Stub Isolation**: The `host_syscall_stub.c` is only allowed when `BHARAT_HOST_TEST` or `BHARAT_BUILD_UNIT_TESTS` is `ON`. It is explicitly forbidden for production runtime images.

## Build Targets

Production targets (e.g., `x86_64-dev`, `arm64-edge`) should always aim for full syscall support. Experimental or constrained 32-bit targets (e.g., `arm32-edge`) should have `BHARAT_ENABLE_USERSPACE` disabled until support is implemented.
