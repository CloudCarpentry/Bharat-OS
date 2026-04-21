---
title: Personality Layer Architecture
status: active
owner: Architecture Team
reviewers: ["Core Team"]
version: 1.1
last_updated: "2026-04-21"
tags: ["architecture", "personalities"]
---

# Personality Layer Model

> **Note on Code Structure:** The architecture enforces a strict separation. Syscalls hit a generic trap layer (`kernel/src/arch/*/trap.c`) and are routed through the native UAPI. Foreign ABIs are supported through compatibility layers (`personalities/compat/linux/`, etc.) that translate POSIX and other semantics into native IPC/uRPC requests.


## 0. Documentation Entry Point

Use `docs/architecture/personalities/README.md` as the canonical index for all personality-layer architecture and roadmap content.

## Overview

The Bharat-OS microkernel provides _no_ POSIX system calls natively. Features like `fork()`, `open()`, network sockets, and user accounts do not exist in Ring-0.

Instead, the OS defines a **Native Bharat Personality** and corresponding UAPI, while relying on **Compat Personality Layers** to present expected environments to legacy applications.

### The Golden Rule of Personalities

> **Kernel mechanisms are native. Services are native. UAPI is native. Compat personalities translate into native.**

## 1. Native Personality (BHARAT_NATIVE)

This is the real OS contract for Bharat-native apps. It does not use POSIX paths or ambient file descriptors.

- **Native UAPI (`uapi/`):** Defines the stable public contracts (process/thread syscalls, capability rights, IPC message layouts, native handles).
- **Capability-Driven:** File access occurs via capability/object handle acquisition, followed by service-mediated lookup.
- **Service Ownership:** `services/system/filesystem/` owns the namespace and mount policies. `stacks/storage/` owns the composed storage internals.

## 2. Compat Personalities (e.g., Linux POSIX)

These translate foreign semantics into the native kernel and service model. They **do not** redefine the kernel core.

Compat adapters should increasingly target the additive modern primitives (`intent_set/get`, class-aware allocation, fault-domain operations) via table-driven mappings, while preserving default legacy behavior when mappings are disabled.

- A legacy Linux application makes a standard `openat()` or `write()` syscall.
- The `compat/linux/` layer catches this and maps the POSIX concepts into native capability-scoped IPC messages.
- These IPC messages are dispatched to the native UAPI endpoints (like the VFS service).

## 3. Unikernel / Library OS (Cloud-Native Mode)

_Optional single-address-space mode for trusted workloads._
In this mode, a single application (e.g., an AI microservice or Nginx) is statically compiled _with_ the Bharat-OS core and the appropriate translation layer into a single executable image.

- There is no user-space separation.
- The application invokes hardware drivers via direct function calls instead of IPC, eliminating all overhead.
- Strictly for Cloud virtualization where the Hypervisor already provides hardware isolation.

## 4. Specific Compatibility Subsystems (Research Horizon)

Subsystems exist to run pre-compiled foreign binaries unmodified while natively leveraging Bharat-OS's advanced security, performance, and scheduling features.

By translating foreign ABIs into Bharat-OS primitives, we allow legacy applications to transparently benefit from the microkernel architecture.

### Linux Subsystem (WSL-like / LKL)

- **Syscall Trapping:** A user-space daemon (the Translator) traps Linux syscalls (`syscall` instruction or `int 0x80`) from the unmodified Linux ELF executable.
- **Mapping Semantics:** The Translator maps Linux semantics directly into Bharat-OS Capability RPCs. For example, Linux file descriptors (FDs) are backed by Bharat-OS Capability tokens pointing to VFS nodes.
- **Advanced Networking & I/O:** `epoll` and `io_uring` requests are seamlessly translated into Bharat-OS's asynchronous Lockless URPC rings, allowing high-throughput Linux web servers to achieve bare-metal multikernel performance.
- **Security Isolation:** Even if a Linux binary is compromised, it operates within a strict Capability CSpace sandbox. It possesses no ambient root privileges, and directory traversal attacks are physically impossible beyond the delegated mount capabilities.

### Windows Subsystem (Wine-like NT Compatibility)

- **PE Loading & ABI:** Emulates Windows Portable Executable (PE) loading and the NT Kernel syscall interface (`ntdll.dll` wrapping).
- **NT Objects to Capabilities:** Windows NT Objects (Handles, Events, Mutexes, Sections) map cleanly to Bharat-OS Kernel Objects. A Windows `HANDLE` becomes a Bharat-OS Capability (`cap_t`).
- **LPC Translation:** The Windows Local Procedure Call (LPC / ALPC) mechanism is perfectly modeled using Bharat-OS Synchronous Endpoints (`ep_t`) for fast cross-process communication.
- **AI Governor Integration:** Windows Thread Priorities and scheduling hints are transparently fed into the Bharat-OS AI Governor (`ai_sched_context_t`), optimizing legacy Windows applications for modern NUMA/Heterogeneous architectures without modification.

### macOS / Darwin Subsystem (Mach Compatibility)

- **Mach Ports & IPC:** Darwin's XNU kernel relies heavily on Mach Ports. These are mapped 1:1 to Bharat-OS IPC Endpoints. Send/Receive rights natively match the Bharat-OS Capability model (`CAP_PERM_SEND`, `CAP_PERM_RECEIVE`).
- **Mach Tasks & Threads:** Mach Task/Thread management maps directly to Bharat-OS `ktask_t` and `kthread_t` primitives.
- **Objective-C / Swift Runtime:** Native Darwin binaries can execute with high performance, as message-passing overhead is minimized via URPC when scaling across cores.

_Note: Full POSIX, Windows, and macOS translations are considered deferred research modules due to the enormous surface area of their respective APIs, but the architectural scaffolding is designed to support them natively as first-class, high-performance environments._
