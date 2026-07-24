# Bharat-OS SDK Syscall Backend Contract

This document defines the requirements and conventions for architecture-specific syscall backends in the Bharat-OS SDK.

## Backend Selection Logic

The SDK selects the appropriate assembly backend based on the target architecture.

1.  **Architecture Capability**: Only architectures with `BHARAT_ARCH_HAS_SDK_SYSCALL_BACKEND` set to `1` in `core/arch/CMakeLists.txt` will attempt to include a real assembly backend.
2.  **Userspace Guard**: If `BHARAT_ENABLE_USERSPACE` is `ON` but no valid architecture backend is found, the build will fail at configure time.
3.  **Host Test Isolation**: If building for host tests (`BHARAT_HOST_TEST` or `BHARAT_BUILD_UNIT_TESTS`), a mock `host_syscall_stub.c` is used instead of assembly backends. This stub **must never** be linked into a production runtime image.

## Symbol Requirements

*   **Exactly One**: Every enabled architecture must provide exactly one public `bharat_syscall` symbol.
*   **Signature**: `long bharat_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);`
*   **Audit**: The `tools/abi/check_syscall_symbols.py` tool is used to verify these requirements and ensure no leakage of test stubs.

## Architecture Conventions

### x86_64
*   **Instruction**: `syscall`
*   **ABI**: `rax` = sysno, `rdi`, `rsi`, `rdx`, `r10`, `r8`, `r9` = args.
*   **Clobbers**: `rcx`, `r11`.

### ARM64
*   **Instruction**: `svc #0`
*   **ABI**: `x8 = sysno`, `x0-x5 = args`.
*   **Return**: `x0`.

### RISC-V 64-bit
*   **Instruction**: `ecall`
*   **ABI**: `a7 = sysno`, `a0-a5 = args`.
*   **Return**: `a0`.

## Implementation Files

*   `experience/user/sdk/lib/src/x86_64_syscalls.S`
*   `experience/user/sdk/lib/src/arm64_syscalls.S`
*   `experience/user/sdk/lib/src/riscv64_syscalls.S`
*   `core/lib/syscall/host_syscall_stub.c` (Host tests only)
