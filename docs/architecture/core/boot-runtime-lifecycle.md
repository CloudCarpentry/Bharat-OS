# Boot and Runtime Lifecycle Contract

## 1. Purpose

The purpose of this document is to define the exact **post-boot runtime behavior** of Bharat-OS. It serves as a normative developer contract, explicitly stating that successful boot is not the end state. Successful boot means the kernel has transitioned into a stable runtime state with a scheduler, interrupts, idle threads, core monitors, and the first system management service active.

This document defines what the kernel must create after boot, what differs by profile, what stays permanently running, what is restartable or optional, and how build/profile selection maps to runtime behavior.

## 2. Scope

This document covers:
* The phase-by-phase transition from early kernel initialization to steady-state runtime.
* The mandatory execution contexts that must exist on all profiles.
* The contract and responsibilities of the first system management service (`sysmgr` / `policy_manager`).
* The classification of long-running services (permanent, restartable, on-demand).
* The rules governing fault handling and service restart policies across profiles.
* How build configurations and hardware profiles interact with runtime behavior.

This document does *not* cover:
* Specific implementations of device drivers.
* Internal algorithms of the scheduler (though it defines scheduler hints by profile).
* Detailed memory allocation internals.

## 3. Definitions

* **Boot Success:** The point at which the kernel has fully initialized hardware primitives, memory management, and the scheduler, and has handed control to the first system management service.
* **Runtime Steady State:** The operational mode where the system policy manager has started all mandatory profile services, and the system is ready to handle external workloads or control loops.
* **Process:** A user-space execution context isolated by the MMU/MPU (where applicable).
* **Service:** A distinct functional entity (e.g., telemetry manager, network stack) that may run in its own process or as an isolated task, governed by the system policy manager.
* **Monitor:** A per-core execution context responsible for observing core health and handling core-local control messages.
* **Profile:** A build-time or boot-time descriptor (e.g., General Purpose, Real-Time, Safety) that governs memory classes, scheduling hints, fault policies, and allowed services.
* **Critical Service:** A service whose failure triggers a domain-wide or system-wide recovery action (e.g., safe mode, reboot) as dictated by the profile.
* **Fault Domain:** An isolated boundary containing one or more services, designed to contain faults and allow local recovery without full system reboot.

## 4. Design Principles

* **Kernel provides mechanism, not policy:** The kernel manages threads, memory, and IPC. Policy decisions (what to start, when to restart, how to handle non-fatal faults) belong in the user-space policy manager (`sysmgr`).
* **One boot model, many profiles:** There is a single, unified boot flow. Different behavior (e.g., "drone mode", "TV mode") is achieved by reading the profile descriptor and starting different services, not by `#ifdef` hacks in the kernel.
* **Per-core ownership and message-driven coordination:** Control, telemetry, power, and safety coordination use disciplined per-core messaging and endpoint registration.
* **Minimal permanent kernel-resident contexts:** The kernel stays small. Rich functionality lives in user-space services.
* **Services are profile-governed:** Services must declare their supported profiles. The system policy manager filters unapproved services at runtime.

## 5. Boot-to-Runtime Phases

The boot sequence must follow this exact phase transition:

1.  **Early Boot:** Architecture setup, stack initialization, early serial/console.
2.  **Hardware Init:** Trap, IRQ, MMU/MPU, and timer subsystem initialization.
3.  **Core Kernel Init:** PMM, VMM, and object capability registries initialized.
4.  **Scheduler Init:** The scheduler is brought online.
5.  **Per-Core Context Creation:**
    *   One idle thread per core is created.
    *   One core monitor per core is created.
6.  **Bootstrap Thread Creation:** A dedicated thread is created to spawn the first user-space service.
7.  **First System Service (`sysmgr`):** The bootstrap thread launches the system policy manager and exits.
8.  **Profile-Based Service Startup:** `sysmgr` reads the profile, queries the subsystem registry, and starts mandatory and optional services.
9.  **Steady Runtime:** `sysmgr` shifts from initialization to supervision (watchdog, telemetry, IPC routing).

## 6. Mandatory Post-Boot Contexts

For *all* profiles, the kernel **must** guarantee the existence of the following execution contexts after boot:

1.  **Idle Thread(s):** One per CPU core.
2.  **Bootstrap/Init Kernel Thread:** One temporary thread used strictly to launch the first user-space manager before self-terminating.
3.  **Core Monitor(s):** One per CPU core. A control context responsible for receiving health queries, IPIs, and core-local maintenance.
4.  **System Policy Manager (`sysmgr`):** The first and primary long-running user-space process.

## 7. First System Service Contract

The kernel **must** start exactly one system management service (`sysmgr` or `policy_manager`) as the first user-space process.

The responsibilities of this manager **must** include:
*   Reading the active profile, board configuration, and hardware capability state.
*   Querying the kernel's subsystem/capability registry.
*   Deciding which services to start based on the profile and subsystem capabilities.
*   Applying security, safety, telemetry, networking, and power mode policies.
*   Supervising the lifecycle (health, restart, crash handling) of all subsequent services.

## 8. Profile-Based Startup Rules

*   **Subsystem Query:** The kernel **may** expose APIs for the policy manager to query profile characteristics.
*   **No Duplicated Policy:** Individual kernel subsystems **must not** contain hardcoded profile policy (e.g., `#if BHARAT_PROFILE_DRONE`).
*   **Filtering:** The policy manager **must** filter subsystem enablement. It **must only** start services that explicitly support the current active profile.
*   **Centralized Authority:** The final authority on what starts and stops belongs exclusively to the `sysmgr`.

## 9. Long-Running Service Classes

Runtime entities are classified into three categories:

### Permanent
These **must** run continuously and cannot be safely terminated without halting the system or entering a critical fault state:
*   Idle thread(s)
*   Core monitor(s)
*   System Policy Manager (`sysmgr`)
*   Watchdog / Health path (for critical, safety, and automotive profiles)

### Restartable
These are designed to crash and be restarted by the `sysmgr` (within the rules of the active fault domain):
*   Telemetry manager
*   Network manager
*   Media manager
*   Storage manager
*   Update manager

### On-Demand
These are launched only when requested by the policy manager or user interaction, and may exit when idle:
*   UI session manager
*   Shell / Debug console
*   AI governor
*   Maintenance tools

## 10. Fault and Restart Behavior

Fault handling is strictly governed by the active profile and defined fault domains.

*   **General Purpose (GP):** **May** freely restart non-critical services (e.g., UI, media) upon crash.
*   **Real-Time (RT):** **May** isolate a crashed service and continue running in a degraded state.
*   **Safety Critical:** **Must** force a transition to a safe mode or trigger a full system reboot upon critical service failure.
*   **Cloud / Datacenter:** **Should** attempt to restart the service while immediately capturing and transmitting telemetry/crash dumps.
*   **Medical:** **Must** generate an immutable audit log of the fault and transition to a hard safe-state.

## 11. Build/Config Interaction

The runtime state is determined by a confluence of configuration variables:
*   **CMake Preset:** Defines the base toolchain and architecture (e.g., ARM64, RISC-V).
*   **Board Configuration:** Defines hardware specifics (memory map, IRQ routing).
*   **Device Profile:** Defines the overarching goal (e.g., `DESKTOP`, `AUTOMOTIVE`, `DRONE`).
*   **Kernel Execution Profile:** Defines scheduler behavior (`GP`, `RT`, `MIX`).
*   **Personality:** Defines user-facing behavior and API surface.

The `sysmgr` uses these parameters to resolve the final runtime policy matrix.

## 12. Developer Rules

When a developer adds a new service or subsystem:
1.  **Descriptor:** The service **must** provide a service descriptor detailing its name, type, init priority, required capabilities, and supported profiles.
2.  **No Kernel Hacks:** The developer **must not** modify the kernel boot path to start their service.
3.  **Registration:** The service **must** register with the subsystem registry so `sysmgr` can discover it.

## 13. Test Requirements

To validate this contract, the test suite **must** assert the following:
*   `[Test-01]` Boot successfully reaches steady-state runtime without panic.
*   `[Test-02]` Exactly one idle thread exists per CPU core.
*   `[Test-03]` The `sysmgr` process starts successfully and assumes PID 1 (or equivalent).
*   `[Test-04]` The policy manager correctly filters and prevents unsupported services from starting based on the active profile.
*   `[Test-05]` All mandatory services for a given profile start successfully.
*   `[Test-06]` A crashed restartable service is successfully restarted by `sysmgr` according to its fault domain policy.
*   `[Test-07]` A crashed critical service in a Safety profile correctly triggers safe mode or reboot.

## 14. Open Issues / Roadmap

*   **Phase 2:** Formalize the exact IPC handshake mechanism between the kernel bootstrap thread and the `sysmgr` executable.
*   **Phase 2:** Implement strict capability dropping for the bootstrap thread before `sysmgr` exec.
*   **Phase 3:** Standardize the `sysmgr` query API for remote/distributed fault domains (e.g., hypervisor supervision).
