---
title: Personalities Roadmap
status: active
owner: Architecture Team
reviewers: ["Core Team"]
version: 1.1
last_updated: "2026-04-22"
tags: ["architecture", "personalities", "roadmap"]
---

# Bharat-OS Personalities Roadmap

This document outlines the phased development roadmap for Bharat-OS personalities, enabling application compatibility for POSIX, Linux, and Android environments on top of the capability-based multikernel architecture.

## Overview

The personality layer provides standard application binary interfaces (ABIs) and system call semantics. By cleanly separating the personality (policy) from the core kernel (mechanisms), Bharat-OS can natively run applications designed for different operating systems without requiring a monolithic global lock or shared state.

All personalities route their requests through `core/kernel/src/core/arch/*/trap.c` which invokes the appropriate, table-driven personality dispatcher (e.g., `core/kernel/src/personality/linux/`).

## Short-Term Milestones (Demoable Targets)

The immediate focus is to achieve minimum viable execution for essential use cases to prove out the multikernel and capability isolation architecture.

### Milestone 1: POSIX / Native Baseline (v0.1)
- **Goal:** Run statically compiled, simple C applications natively on Bharat-OS.
- **Deliverables:**
  - Basic standard C library (libc/musl) ported as the native user-space runtime.
  - VFS integration (console read/write, simple RAMFS open/close).
  - Basic memory mapping (anonymous `mmap`, `brk`).
  - Thread lifecycle management (`exit`, `gettid`).
  - Standard error code mapping (`include/bharat/errno.h`).

### Milestone 2: Minimal Linux Personality (v0.2)
- **Goal:** Boot a pre-compiled Linux binary (e.g., busybox, statically linked) on a Bharat-OS node.
- **Deliverables:**
  - Pluggable Linux Syscall ABI trap handler (`core/kernel/src/personality/linux/`).
  - Translation of core Linux syscalls to Bharat-OS primitives:
    - Process/Thread: `clone` (thread-first), `execve`, `exit_group`.
    - Memory: `mmap` (anonymous), `munmap`, `mprotect`.
    - Filesystem: `openat`, `read`, `write`, `close`, `lseek`.
  - Stubbing of unimplemented/unnecessary Linux syscalls to gracefully return `-ENOSYS`.

### Milestone 3: Android Execution Stub (v0.3)
- **Goal:** Provide a basic execution environment capable of loading an Android command-line binary.
- **Deliverables:**
  - Setup of `subsys/android/` wrapping the Linux baseline.
  - Implement basic Bionic libc compatibility requirements (TLS, memory layout).
  - Dummy/stub implementations of specific Android properties (e.g., `/dev/__properties__`).

## Long-Term Milestones (Harder Goals)

These milestones focus on deeper integration, high-performance concurrency, and full-stack subsystem support.

### Milestone 4: Advanced Linux Personality (v1.0)
- **Goal:** Full support for complex Linux server applications (e.g., Redis, Nginx) utilizing network and IPC.
- **Deliverables:**
  - POSIX Networking stack integration (BSD socket API translation to Bharat-OS Network services).
  - Full `fork()` support leveraging VMM copy-on-write (COW) and capability inheritance.
  - Signal delivery mechanism bridging Bharat-OS async IPC to Linux signal frames.
  - Pseudoterminal (`pty`) and advanced TTY subsystem for interactive shells.
  - Procfs (`/proc`) and Sysfs (`/sys`) synthetic filesystem translations.

### Milestone 5: Full Android Personality (v2.0)
- **Goal:** Boot Android services (SurfaceFlinger, SystemServer) and basic applications without modifications.
- **Deliverables:**
  - Distributed Binder IPC: Rebuild Binder as a distributed IPC fabric leveraging Bharat-OS uRPC and capability tokens instead of monolithic shared memory.
  - Ashmem/Gralloc support mapped to the core memory management subsystems.
  - Hardware Abstraction Layer (HAL) pass-through: Allowing Android user-space HALs to interact securely with Bharat-OS drivers via capability-gated IPC.
  - Zygote process spawning utilizing optimized multikernel memory sharing.
  - SELinux compatibility layer mapped to Bharat-OS capabilities.


## Cross-Architecture Priority Track (x86_64, arm64, riscv64)

To deliver Linux and Android personalities as production-grade features, all roadmap milestones must now be validated across three primary architectures:

- x86_64
- arm64
- riscv64

Execution and ownership details are tracked in `multi-arch-personality-roadmap.md`. This document is now the source of truth for ISA-specific bring-up tasks, KPI gates, and phased release criteria.

## Code Structure Constraints

When contributing to personalities, adhere to these structural rules:
1. **No Core Taint:** Personality-specific structures (e.g., Linux file descriptors, Android Binder nodes) must not leak into core subsystems (`core/kernel/src/vfs/`, `core/kernel/src/mm/`).
2. **Table-Driven Dispatch:** Syscalls must use generic dispatch tables (`g_syscall_table`) indexed by ABI numbers, avoiding massive `switch` statements.
3. **Unified Errors:** Personality code maps local errors (e.g., Linux's `-EINVAL`) to the unified core `include/bharat/errno.h` using standard POSIX names prefixed with `BH_`.
