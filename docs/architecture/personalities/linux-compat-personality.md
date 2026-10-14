---
title: Linux Compatibility Personality
status: active
owner: Architecture Team
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - personalities
see_also:
  - README.md
version: 1.0
---
# Linux Compatibility Personality

The Linux Compatibility Personality provides a POSIX-like user space to accelerate ecosystem unlock for developer tools, existing C/C++ projects, and headless server workloads (e.g., Python, Node.js).

## 1. Scope and Purpose

*   **Goal:** Enable rapid porting of POSIX software with zero or minimal source modifications.
*   **Target Workloads:** CLI tools, server applications, language runtimes (Python, Node.js, early Java/CoreCLR), and build systems.

## 2. ELF Loading Model

*   The personality intercepts execution of statically or dynamically linked Linux ELF binaries.
*   It implements a custom `ld.so` or traps the entry point to inject the libc shim layer.

## 3. POSIX and Syscall Coverage Scope

*   **Hardware Trap & Syscall ABI:** Handled via unified architecture trap extraction across all 5 ISAs (`x86_64`, `arm64`, `arm32`, `riscv64`, `riscv32`). See [`linux-android-syscall-arch.md`](./linux-android-syscall-arch.md) for full instruction, cause vector, and register mappings.
*   **File & Directory I/O:** `open`, `openat`, `read`, `write`, `close`, `seek`, `stat`, `fstat`, `lstat`, `newfstatat`, `access`, `faccessat`, `getcwd`, `chdir`, `dup`, `dup2`, `dup3`, `fcntl`, `getrandom`, `readlink`, `readlinkat`.
*   **IPC & Multiplexing:** `pipe`, `pipe2` (bounded ring buffer, EOF on writer close, EPIPE on reader close), `eventfd`, `eventfd2` (counter, `EFD_SEMAPHORE`), `timerfd_create`, `timerfd_settime`, `timerfd_gettime` (monotonic tick accumulation), `epoll_create`, `epoll_create1`, `epoll_ctl`, `epoll_wait`, `epoll_pwait`, `poll`, `ppoll`, `socketpair`.
*   **Virtual Procfs & Device Nodes:** In-memory `/proc` synthetic filesystem (`/proc/self/status`, `/proc/self/cmdline`, `/proc/self/maps`, `/proc/cpuinfo`, `/proc/meminfo`, `/proc/version`, `/proc/mounts`) and pseudoterminal stubs (`/dev/ptmx`, `/dev/pts/*`, `/dev/null`, `/dev/zero`, `/dev/urandom`).
*   **Process & Threading:** `getpid`, `gettid`, `clone` (with TLS, TID pointers), `fork`, `vfork`, `execve`, `futex` (wait/wake buckets), `arch_prctl`, `prlimit64`, `wait4`.
*   **Memory:** `mmap`, `munmap`, `mprotect`, `brk` (translated to Bharat-OS VM abstractions).
*   **Signals:** Synchronous signal framework (`rt_sigaction`, `rt_sigprocmask`, `rt_sigreturn`).

## 4. What is Explicitly NOT Supported

This personality does **not** aim for 100% bug-for-bug Linux kernel internal emulation.
*   **No `ptrace` or deep kernel tracing:** Linux specific debug APIs are not supported. Debugging goes through the Bharat-OS diagnostic SDK.
*   **No `cgroups` or `namespaces`:** Linux container models are not emulated. Isolation is handled natively via Bharat-OS capabilities.
*   **No raw device nodes:** `/dev/sda` or raw `ioctl` pass-through to hardware is forbidden. Device access is capability-mediated.

## 5. libc Strategy

*   The personality relies on a custom or heavily patched libc (e.g., musl libc ported to the Bharat-OS UAPI) acting as a bridge.
*   System calls are intercepted in user space and translated to Bharat-OS Native IPC messages to the VFS or Network services.

## 6. Filesystem and Networking Expectations

*   **VFS Bridge:** The personality presents a fake global filesystem hierarchy (like `/etc`, `/usr`, `/tmp`) backed by a chroot-like capability granted by the Native OS.
*   **Networking:** Standard BSD sockets are supported via a library translation layer communicating with the Native network service.
