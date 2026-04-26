---
title: Architecture Decision Record: Boot and Runtime Lifecycle Contract
status: Accepted
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - adr
see_also:
  - README.md
---
# Architecture Decision Record: Boot and Runtime Lifecycle Contract

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: 🚧 Pending core/kernel/service behavior merge
- **Validated**: ❌ Pending stress/fault-injection tests


## Context and Problem Statement

As Bharat-OS targets multiple hardware profiles ranging from tiny embedded control loops to complex cloud gateways, the kernel initialization path historically risked becoming cluttered with profile-specific `ifdef` logic (e.g., `#ifdef BHARAT_PROFILE_DRONE`).

This leads to:
1.  **Code Bloat:** A monolithic kernel that tries to understand every use case.
2.  **Coupling:** Subsystems (like network or filesystem) needing to know about "drone mode" or "medical mode".
3.  **Vague Boot Success:** The definition of "boot success" varying wildly depending on the profile (e.g., simply reaching a shell vs. starting a real-time scheduler and verifying watchdog health).

We needed a unified, scalable way to define how the system transitions from early hardware initialization into a steady-state runtime across any profile, without hardcoding domain-specific knowledge into the core kernel.

## Decision

We will implement a unified boot-to-runtime transition model characterized by:

1.  **One Kernel Boot Model:** All profiles share the exact same sequence of architectural initialization, memory bring-up, and scheduler activation.
2.  **One Profile Descriptor:** The desired system state (e.g., General Purpose, Safety, Real-Time) is represented by a single compiled descriptor or configuration structure (`profile.h` / `subsystem_profile.h`), not scattered macros.
3.  **One Subsystem Registry:** Subsystems and services register their capabilities and supported profiles centrally.
4.  **One Policy-Driven Startup Flow:** The kernel exits initialization by starting exactly one user-space execution context: the System Policy Manager (`sysmgr`).
5.  **No Domain-Specific Hacks:** The core kernel does not know what a "drone" is. It only knows about memory policies, fault domains, and scheduler classes dictated by the profile descriptor.

## Status

Accepted

## Consequences

### Positive

*   **Clean Kernel:** The core kernel is isolated from user-space policy.
*   **Reusability:** Primitives like fault domains, power hooks, and telemetry contracts can be reused across vastly different products (e.g., a network appliance and a desktop).
*   **Determinism:** The exact state of a system post-boot is knowable and predictable based on the profile matrix.
*   **Testability:** End-to-end tests can reliably verify that unsupported services are rejected and mandatory services are started based on the active profile descriptor.

### Negative

*   **Complexity for Simple Devices:** A bare-metal developer cannot simply put their application logic directly into the kernel's `main()`. They must write a proper service and register it with the subsystem registry to be invoked by `sysmgr`.
*   **Overhead:** There is a slight boot latency overhead to spawn the `sysmgr` and query the registry compared to statically hardcoding function calls.

## Alternatives Considered

*   **Monolithic Compile Flags:** Continuing to use `#ifdef BHARAT_PROFILE_DRONE` throughout the codebase. Rejected due to scaling issues and violation of the mechanism vs. policy principle.
*   **Multiple Kernel Entry Points:** Having `kernel_main_drone()` and `kernel_main_desktop()`. Rejected because it forks the core initialization logic and makes maintenance of core primitives (like MMU/PMM) error-prone.
*   **Scripted Init (like init.d or systemd):** Relying entirely on a shell script parser at boot. Rejected because many of our target profiles (tiny embedded, safety critical) do not have a filesystem or a shell, requiring a compiled, programmatic manager (`sysmgr`).
