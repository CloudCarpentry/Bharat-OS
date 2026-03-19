# ADR 009: SDK & Libc Strategy for Bharat-OS

## Status

Accepted

## Context

Bharat-OS aims to target edge devices, drones, gateways, robotics nodes, and appliance-class systems. A critical hurdle to becoming a testable OS platform has been the "developer entry cost". Without a standard C runtime (libc) and Software Development Kit (SDK), porting utilities, benchmarks, third-party libraries, and writing standard tests are unnecessarily complex, as each component ends up reinventing the wheel (e.g., custom `stdio`, `string` routines, custom memory allocators).

While POSIX compatibility is ultimately desired for code reuse and developer familiarity, building a monolithic, fully-featured POSIX implementation (similar to a glibc-based desktop Linux system) from day one is not the right fit for the multikernel, capability-driven architecture of Bharat-OS. Pushing POSIX semantics (like `fork`, signals, and implicit shared state) deep into the core kernel violates the principle that the kernel should export **mechanisms only**.

Furthermore, attempting to write a full libc from scratch is a significant time sink and risks distracting from core architecture goals.

## Decision

We will adopt a **tiered, mechanism-first SDK/libc approach**, rather than a monolithic POSIX kernel. This strategy comprises:

1. **Small, deterministic libc first.** A Core SDK libc and runtime layer that provides basic memory, string, `stdio`, `errno`, and standard C headers. This unlocks immediate value for embedded apps, kernel services, and test harnesses.
2. **Selective POSIX compatibility second.** A POSIX library shim that implements an embedded subset (like `read`, `write`, `open`, `close`, threads, and poll) over the core SDK's syscall/IPC veneers. We will explicitly avoid complex, monolithic Unix semantics like full `fork()` initially, preferring `spawn`-based process models.
3. **Personality layers third.** Behavioral compatibilities (e.g., Linux, RT/Embedded, Appliance, Drone) will live above the kernel and libc, mapping API behaviors, `errno` values, and subsystem expectations as independent runtime profiles.

Architecturally, the kernel will remain focused on capabilities, address spaces, endpoint IPC, and threads. The libc will communicate with the kernel strictly via a stable **Bharat UAPI** (e.g., a `libbsys` layer), ensuring that POSIX remains a translation/adaptation layer.

### Source Strategy: Hybrid Two-Step Approach

Instead of writing a libc from scratch, we will implement a two-step adoption plan:

* **Stage 1 (Minimal SDK Bootstrap):** We will use a `newlib` or `picolibc`-style minimal approach to establish the early SDK. This provides very fast embedded bring-up, allowing services and tests to compile cleanly without custom headers.
* **Stage 2 (Hosted libc model):** We will eventually converge toward a `musl`-compatible hosted libc model for richer POSIX support. Musl is small, clean, and a better embedded fit than glibc, making it easier to adapt to our custom kernel.

## Consequences

### Positive

* **Faster bring-up:** Immediate ability to compile tests, services, and embedded apps using standard standard C headers and minimal `stdio`/`string` routines.
* **Architectural purity:** The kernel avoids being polluted with heavyweight, monolithic POSIX semantics like `fork` and complex signal handling. POSIX is strictly an emulation/shim layer.
* **Clear progression:** The two-stage adoption (newlib/picolibc style -> musl style) gives a realistic roadmap without committing to an overwhelming initial effort.
* **Targeted usage:** By separating POSIX subsets and personalities, we can tune the OS for drones, edge appliances, or generic porting, without forcing one size to fit all.

### Negative

* **Migration overhead:** We must plan for the eventual transition from Stage 1 (newlib/picolibc-style) to Stage 2 (musl-style).
* **Emulation complexity:** Shimming POSIX over capabilities requires careful design of the internal UAPI (`libbsys`) and userspace service interaction (like file descriptor models mapping to VFS handles). Porting complex desktop software will be challenging until Phase 6 of the implementation plan.
* **No `fork()`:** Applications relying heavily on `fork()` will need to be rewritten or adapted to use `posix_spawn()` or similar task-creation interfaces, which may slow down the porting of some legacy Unix tools.
