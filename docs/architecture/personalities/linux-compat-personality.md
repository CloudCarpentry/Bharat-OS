---
title: Linux Compatibility Personality
status: active
owner: Architecture Team
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

*   **File I/O:** `open`, `read`, `write`, `close`, `seek`, `stat`.
*   **Memory:** `mmap`, `munmap`, `mprotect` (translated to Bharat-OS VM abstractions).
*   **Sockets:** Baseline IPv4/IPv6 TCP and UDP sockets.
*   **Threading:** Standard `pthread` expectations (creation, mutexes, condition variables) mapped to Bharat-OS native threads.
*   **Signals:** A restricted, mostly synchronous subset of signals (e.g., `SIGINT`, `SIGTERM`, `SIGSEGV`).

## 4. What is Explicitly NOT Supported

This personality does **not** aim for 100% bug-for-bug Linux compatibility.
*   **No full `fork()`:** Applications relying on complex `fork()`+`exec()` states with deep copy-on-write assumptions may fail or require porting to `posix_spawn()`.
*   **No `ptrace` or deep kernel tracing:** Linux specific debug APIs are not supported. Debugging goes through the Bharat-OS diagnostic SDK.
*   **No `cgroups` or `namespaces`:** Linux container models are not emulated. Isolation is handled natively via Bharat-OS capabilities.
*   **No raw device nodes:** `/dev/sda` or raw `ioctl` pass-through to hardware is forbidden.

## 5. libc Strategy

*   The personality relies on a custom or heavily patched libc (e.g., musl libc ported to the Bharat-OS UAPI) acting as a bridge.
*   System calls are intercepted in user space and translated to Bharat-OS Native IPC messages to the VFS or Network services.

## 6. Filesystem and Networking Expectations

*   **VFS Bridge:** The personality presents a fake global filesystem hierarchy (like `/etc`, `/usr`, `/tmp`) backed by a chroot-like capability granted by the Native OS.
*   **Networking:** Standard BSD sockets are supported via a library translation layer communicating with the Native network service.
