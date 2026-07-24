---
title: Personality Model
status: active
owner: Architecture Team
reviewers: ["Core Team"]
version: 1.0
last_updated: "2024-03-23"
tags: ["architecture", "personalities"]
---

# Personality Compatibility Model

> **Note on Code Structure:** Following recent architectural convergence, the personality layer is explicitly separated from the core architecture. Implementations reside in the `core/personalities/` tree (with sub-directories like `compat/linux/`, `compat/android/`), mapping specific ABIs to core operations exposed via `interface/uapi/`.

In a modern microcore/kernel/multikernel architecture like Bharat-OS, forcing a single monolithic application binary interface (ABI) across all workloads limits flexibility and drags unnecessary state into the kernel.

Instead, Bharat-OS defines a **Native Bharat Personality** and supports **compat personalities** layered above the core capability-driven kernel primitives. This document outlines the distinct personality compatibilities we aim to implement to support different deployment profiles.

## Core Principle

**Kernel mechanisms are native. Services are native. UAPI is native. Compat personalities translate into native.**

The kernel does not define a "POSIX process model" natively. It exports address spaces, threads, capability handles, endpoint IPC, timers, VM map/unmap, and per-core execution models. Compat personalities translate these primitives into expected semantics for different application classes.

---

## Personality Types

### 1. Native Personality (`BHARAT_NATIVE`)
**Best for:** First-class Bharat-OS native applications, highly secure edge daemons, and low-latency multikernel services.

This is the default, native OS contract for Bharat-OS apps. It is not Linux-compatible; it is capability-oriented and secure by default.

* **Maps:**
  * Capability/object handle acquisition.
  * Namespace or service-mediated open/lookup via IPC.
  * Capability-scoped read/write/map/stat-like operations.
  * Process create/destroy and thread create/exit/join primitives directly to UAPI.
  * Sandboxing enforced by capability possession and rights narrowing.
* **Architecture Placement:** Uses the `interface/uapi/` explicit boundary for syscall headers, capability contracts, shared IPC structures, and stable ABI types.

### 2. Linux Personality (Compat Layer)
**Best for:** Developer adoption, general-purpose porting, tools, and testing.

This personality is the first to build and aims to provide the familiar "Linux-like" behavior developers expect when cross-compiling or porting generic code.

* **Maps:**
  * Linux `errno` values (e.g., specific error codes returned by failed file operations).
  * Linux flags and constants (e.g., `O_CLOEXEC`, `mmap` flags).
  * Linux-ish file descriptor semantics (e.g., inheritance, duping).
  * `pthread` expectations for synchronization and thread lifecycle.
  * `mmap`/`prot`/flags translation down to Bharat-OS VM abstractions.
  * A subset of socket `ioctls` (later).
* **Limitations:** Does not promise 100% bug-for-bug compatibility with glibc/Linux internals. Heavyweight concepts like full `fork()` and complex signals are shimmed, stubbed, or deferred.

### 3. Embedded / RT Profile (Domain Personality)
**Best for:** Drones, robotics, edge control, and deterministic workloads.

This personality focuses on predictable execution, strict bounds, and safety-critical patterns.

* **Maps:**
  * Deterministic startup and capability initialization.
  * Fixed memory arenas (eschewing or strictly bounding dynamic `malloc`).
  * Reduced signal complexity (or strictly synchronous exception delivery).
  * Strict priority threading and affinity APIs.
  * Watchdog and fault hooks deeply integrated into the runtime.
  * **No `fork`/`exec` assumption:** Task creation is explicit, clone-like, or strictly capability-scoped `spawn`.

### 4. Appliance Profile (Domain Personality)
**Best for:** Routers, gateways, storage/network appliances.

This personality bridges the gap between full general-purpose Linux and strict embedded RT, providing enough dynamic capability to run daemons while restricting arbitrary execution.

* **Maps:**
  * Service lifecycle management (launching, restarting, dependency tracking).
  * Capability-limited filesystem and network access (e.g., a sandboxed daemon).
  * Persistent configuration model exposed uniformly to applications.
  * Low-footprint logging and crash capture semantics (tailored for reliable edge telemetry).

## Architectural Placement

Compat personalities reside as services or adapters built around the UAPI:
* `core/kernel/` -> `interface/uapi/` (Bharat-native ABI) -> `core/personalities/compat/linux/`

For example, a legacy application compiled with a Linux toolchain will interact with the Linux compat personality layer (`compat/linux/`), which translates standard POSIX `openat()` or `read()` operations down into capability-scoped, native UAPI IPC messages sent to `core/services/system/filesystem/`.
