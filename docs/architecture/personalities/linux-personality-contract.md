---
title: Linux Personality Contract
status: active
owner: Architecture Team
version: 1.0
---

# Linux Personality Contract

This document defines the ABI boundary and performance contract for the Linux personality on Bharat-OS. The Linux personality is an external ABI compatibility layer; it maps Linux concepts to Bharat-OS core mechanisms without leaking policy into the kernel.

## A. ABI Scope

The Linux personality will not support the full Linux syscall table immediately. It will be built upon a minimal, performance-grade slice:
*   **Syscall Coverage Strategy:** Focus initially on a minimal runnable slice (e.g., `read`, `write`, `close`, `mmap`, `munmap`, `mprotect`, `futex`, `clock_gettime`, `epoll` subset, `exit`, and thread `clone`). Full compatibility will expand outward only after performance targets are validated.
*   **ELF Expectations:** The system must efficiently handle ELF interpreter contracts, correct aux vector (`auxv`) population, and standard stack layouts.
*   **Thread & Signals:** Robust handling of TLS initialization (via standard ABI conventions), thread creation, and a lightweight signal mapping model that routes directly to the kernel's notification core.

## B. Performance Rules

The goal is **near-native semantics and performance**.
*   **Direct Syscall Mapping:** Syscalls classified as hot paths (e.g., `mmap`, `futex`, `read`, `write`, `epoll`) must use direct mapping or zero-copy translation into kernel mechanisms. Emulation is reserved strictly for slow paths and legacy compatibility.
*   **Near-Native Hot Paths:** `mmap`, `futex`, and `epoll` implementations must achieve latency parity with native Linux. This includes using shared kernel primitives (like user-memory wait/wake primitives) without a translation tax.
*   **No Overhead on Internal Operations:** Avoid per-syscall object translation loops or repeated personality/string lookups. The process/thread descriptors must carry the personality ID and relevant ABI/TLS configurations to allow immediate dispatch.

## C. Constraints (What NOT to do)

*   **No Kernel Policy Leakage:** The Linux personality lives in the ABI compatibility layer. Policy decisions (like complex namespaces or cgroups emulation) belong in user-space services, not the kernel core.
*   **No Double-Layer VM Handling:** The VM mapping path must be single-authority. Do not maintain a separate "Linux VMA list" that mirrors a "Bharat VMA list." A single kernel VM mechanism must serve both with zero-cost abstraction.
*   **No Object Translation Loops:** Avoid mapping integer FDs to internal object references on every hot path operation with expensive lock-contended hash lookups. Use direct pointer handles securely or highly optimized, lockless, per-process FD tables.

## D. Layering Alignment

The Linux personality must respect the established repository structure and architectural boundaries:
*   **Kernel (`kernel/`):** Mechanism only. Contains generic trap entry, scheduling, VM, IPC, and primitive wait/wake.
*   **Services (`services/`):** Policy orchestration. Manages permissions, lifecycle, and higher-level OS constructs.
*   **Personalities (`personalities/compat/linux/`):** ABI mapping only. Adapts Linux userland calls to the core kernel and service mechanisms without bringing its own parallel OS logic.
*   **Android Relationship:** The Android personality is strictly defined as "Linux personality + Android Delta" (e.g., Binder, Ashmem, Bionic differences). It does not duplicate Linux functionality.
