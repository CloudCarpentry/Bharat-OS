# Personality Compatibility Model

In a modern microkernel/multikernel architecture like Bharat-OS, forcing a single monolithic application binary interface (ABI) across all workloads limits flexibility and drags unnecessary state into the kernel.

Instead, Bharat-OS supports **behavioral personalities** layered above the core capability-driven kernel primitives. This document outlines the distinct personality compatibilities we aim to implement to support different deployment profiles.

## Core Principle

**Kernel = Mechanisms. Libc / POSIX / Personality = Policy & Compatibility.**

The kernel does not define a "POSIX process model" natively. It exports address spaces, threads, capability handles, endpoint IPC, timers, VM map/unmap, and per-core execution models. Personalities map these primitives into expected semantics for different application classes.

---

## Personality Types

### 1. Linux Personality
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

### 2. Embedded / RT Personality
**Best for:** Drones, robotics, edge control, and deterministic workloads.

This personality focuses on predictable execution, strict bounds, and safety-critical patterns.

* **Maps:**
  * Deterministic startup and capability initialization.
  * Fixed memory arenas (eschewing or strictly bounding dynamic `malloc`).
  * Reduced signal complexity (or strictly synchronous exception delivery).
  * Strict priority threading and affinity APIs.
  * Watchdog and fault hooks deeply integrated into the runtime.
  * **No `fork`/`exec` assumption:** Task creation is explicit, clone-like, or strictly capability-scoped `spawn`.

### 3. Appliance Personality
**Best for:** Routers, gateways, storage/network appliances.

This personality bridges the gap between full general-purpose Linux and strict embedded RT, providing enough dynamic capability to run daemons while restricting arbitrary execution.

* **Maps:**
  * Service lifecycle management (launching, restarting, dependency tracking).
  * Capability-limited filesystem and network access (e.g., a sandboxed daemon).
  * Persistent configuration model exposed uniformly to applications.
  * Low-footprint logging and crash capture semantics (tailored for reliable edge telemetry).

## Architectural Placement

These personalities reside as libraries or user-space services, forming the top layer of the Bharat-OS SDK:
* `libsys` (Base UAPI) -> `libc` (Core runtime) -> `libposix` (POSIX subset) -> **`libpersonality-*`**

For example, a service compiled with the Linux personality will link against `libpersonality-linux.a`, which intercepts or translates calls (like `open`, `ioctl`, or specific `mmap` requests) before passing them down through the POSIX shim or directly to `libsys` endpoint IPC.
