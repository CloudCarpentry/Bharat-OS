---
title: Linux and Android Syscall Architecture across Target ISAs
status: Active
owner: Personalities & Architecture Working Group
last_updated: 2026-10-14
tags:
  - docs
  - architecture
  - personalities
  - syscall
  - linux
  - android
see_also:
  - docs/architecture/syscall/arch-register-abi.md
  - docs/architecture/personalities/linux-compat-personality.md
  - docs/architecture/personalities/android-compat-personality.md
version: 1.0
---

# Architectural Analysis: Linux & Android Syscalls across Target ISAs

In Bharat-OS, user-space system calls enter the kernel via architecture-specific trap vectors and are normalized into the architecture-neutral `bh_syscall_ctx_t` context before dispatch to personality tables.

---

## 1. Hardware Syscall Register & Instruction Mappings

| Architecture | CPU Instruction | Trap Vector / Cause | Syscall NR Reg | Argument Registers (1 to 6) | Return Value Reg | Bharat-OS Kernel Extractor |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **x86_64** | `syscall` | MSR `LSTAR` (0xC0000082) | `rax` | `rdi`, `rsi`, `rdx`, `r10`, `r8`, `r9` | `rax` | [`core/arch/x86/x86_64/syscall_extract.c`](../../../core/arch/x86/x86_64/syscall_extract.c) |
| **arm64 (AArch64)** | `svc #0` | `ESR_EL1.EC = 0x15` (`ARM64_EC_SVC64`) | `x8` | `x0`, `x1`, `x2`, `x3`, `x4`, `x5` | `x0` | [`core/arch/arm/arm64/syscall_extract.c`](../../../core/arch/arm/arm64/syscall_extract.c) |
| **arm32 (ARM EABI)**| `svc 0` | Sync Trap Vector `0x08` (Cause 8) | `r7` | `r0`, `r1`, `r2`, `r3`, `r4`, `r5` | `r0` | [`core/arch/arm/arm32/syscall_extract.c`](../../../core/arch/arm/arm32/syscall_extract.c) |
| **riscv64 (RV64GC)**| `ecall` | `scause = 8` (U-mode Environment Call) | `a7` (`x17`) | `a0`, `a1`, `a2`, `a3`, `a4`, `a5` (`x10..x15`) | `a0` (`x10`) | [`core/arch/riscv/riscv64/syscall_extract.c`](../../../core/arch/riscv/riscv64/syscall_extract.c) |
| **riscv32 (RV32I)** | `ecall` | `mcause` / `scause = 8` | `a7` (`x17`) | `a0`, `a1`, `a2`, `a3`, `a4`, `a5` (`x10..x15`) | `a0` (`x10`) | [`core/arch/riscv/riscv32/syscall_extract.c`](../../../core/arch/riscv/riscv32/syscall_extract.c) |

---

## 2. End-to-End Syscall Dispatch Pipeline

The trap handling path unifies all hardware architectures into an isolated, capability-verified execution flow:

```
[User-space Application / Personality Binary (Linux ELF, BusyBox, Android APK)]
                                 │
                                 ▼ (Hardware Syscall Trap: syscall / svc / ecall)
                     [Architecture Trap Handler]
                       (trap_entry.S / trap.c)
                                 │
                                 ▼ (arch_trap_extract_syscall)
                   [Normalized Register Context]
                     bh_syscall_regs_t:
                       • nr: Syscall Number
                       • arg[0..5]: Normalized 64-bit/32-bit Arguments
                                 │
                                 ▼ (bh_syscall_gate)
                   [Syscall Capability Gate & Routing]
                     • Validates process personality tag (BH_PERSONALITY_LINUX / ANDROID)
                     • Populates thread & process context pointers
                     • Enforces capability rights & bounds (W^X, memory protections)
                                 │
                                 ▼
                   [Personality Syscall Dispatch Table]
                     • Linux Table: personality_linux_get_table()
                     • Android Table: personality_android_get_table()
                                 │
                                 ▼
                     [Subsystem Handler Execution]
                     • linux_sys_openat / read / write / futex / clone / arch_prctl
                     • Returns bh_operation_result_t
                                 │
                                 ▼
                     [Status & Errno Normalization]
                     • Translates kstatus_t to -errno via linux_errno_from_bh_status()
                                 │
                                 ▼ (arch_trap_set_syscall_return)
                   [Hardware Return Value Written to Frame]
                     • rax (x86_64) / x0 (arm64, arm32) / a0 (riscv64, riscv32)
                                 │
                                 ▼ (sysret / eret / sret / mret)
                    [Resume User-space Execution]
```

---

## 3. How Linux Syscalls Work across Architectures

1. **Standard POSIX/Linux Syscall Conventions:**
   - Linux executables use the standard system call conventions for their compile target.
   - For example, an `aarch64` binary executing `getpid` places `172` in `x8` and executes `svc #0`.
   - On `x86_64`, `getpid` places `39` in `rax` and executes `syscall`.
   - On `riscv64`, `getpid` places `172` in `a7` and executes `ecall`.
2. **File Descriptor Isolation:**
   - Each process owns an isolated `linux_process_state_t` attached to `bh_process_t->personality_data`.
   - FDs (`0`, `1`, `2`, and dynamic descriptors) are mapped directly to underlying Bharat-OS VFS capability handles or character devices.
3. **Threading & TLS:**
   - Thread creation via `SYS_CLONE` maps flags (`CLONE_VM`, `CLONE_THREAD`, `CLONE_FS`) to Bharat-OS `thread_create_detached`.
   - Thread-local storage (TLS) pointer initialization:
     - `x86_64`: initialized via `arch_prctl(ARCH_SET_FS, addr)` (updates MSR `0xC0000100` or process thread state).
     - `arm64`: initialized via `TPIDR_EL0` register write.
     - `arm32`: initialized via `CP15` register `c13` (User Read/Write Thread ID).
     - `riscv64` / `riscv32`: initialized via `tp` register (`x4`).
4. **Futex Synchronization:**
   - User-space locks (e.g. `pthread_mutex`) perform atomic compare-and-swap in user space and trap to `SYS_FUTEX` only upon contention.
   - Kernel hashes the user virtual address to a futex bucket and blocks the calling thread on a dedicated wait queue until `FUTEX_WAKE`.

---

## 4. How Android Syscalls Work

Android user-space applications (compiled against the Android NDK or executing within ART) interact with the operating system through Bionic libc:

1. **Syscall ABI Identity:**
   - Bionic libc emits the **exact same system call instructions and register ABIs** as standard Linux for the given CPU architecture.
   - A call to `mmap` or `clock_gettime` from an Android app has the exact same trap layout as a call from a Debian GNU/Linux binary on that same architecture.
2. **Android-Specific Virtual Device Extensions:**
   - Instead of creating novel system calls, Android extends the operating system through virtual character device interfaces handled via `openat`, `ioctl`, and `mmap`:
   - **Binder (`/dev/binder`):**
     - User space issues `ioctl(fd, BINDER_WRITE_READ, &bwr)`.
     - In Bharat-OS, the Android personality intercepts Binder ioctl operations and maps Binder transactions to per-core uRPC capability rings and HMEM shared buffers for zero-copy IPC.
   - **Ashmem (`/dev/ashmem`):**
     - User space issues `ioctl(fd, ASHMEM_SET_SIZE, size)` and then maps the region with `mmap`.
     - In Bharat-OS, Ashmem is backed by first-class `CAP_TYPE_HMEM` capability objects, eliminating redundant kernel page cache copies.
   - **Android Properties (`/dev/__properties__`):**
     - Global system property storage is presented as a read-only memory-mapped shared buffer, matching Android's `libcutils` expectation.
   - **Logcat (`/dev/log` or UNIX domain socket `/dev/socket/logd`):**
     - Log events are forwarded to the system diagnostics and observability daemon.

---

## 5. Security Invariants & Fail-Closed Boundaries

1. **No Raw Pointer Leakage:** Syscall handlers must never accept or return raw kernel memory pointers across user-space boundaries. All data transfers must pass through `bh_copy_from_user` and `bh_copy_to_user`.
2. **Fail Closed:** Any unknown or unsupported system call number returns `-LINUX_ENOSYS` (`-38`) and records an audit diagnostic.
3. **CSpace Isolation:** Capabilities cannot be forged from raw integers; all capability tokens must be resolved through the invoking process's capability table (`CSpace`).
