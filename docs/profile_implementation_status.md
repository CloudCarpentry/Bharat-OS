# Bharat-OS Profile Implementation Status & Gap Analysis

This document provides a deep implementation-status and gap-analysis review of the Bharat-OS kernel, specifically cross-referencing `docs/profile_priorities_and_kernel_changes.md`. It places special emphasis on the **small device**, **drone**, and **automobile** production readiness.

## 1. Structured Status Review of Subsystems

### Memory Management (MM)
* **Status:** Implemented but incomplete
* **Details:** The PMM (Physical Memory Manager) and VMM (Virtual Memory Manager) with cacheability properties are reasonably structured in `kernel/src/mm`. Memory classes like zswap are present. However, static pool allocation—critical for RTOS/safety profiles to avoid heap dependency in critical paths—is weak or missing proper generic enforcement across the HAL.

### Scheduler / Real-Time Behavior
* **Status:** Implemented but incomplete
* **Details:** The scheduler (`kernel/src/sched.c`) implements an O(1) wait queue embedded in thread structs (no kmalloc) and supports policies like `SCHED_POLICY_ROUND_ROBIN`, `SCHED_POLICY_PRIORITY`, etc., which satisfies core constraints. AI scheduler infrastructure (`ai_sched.c`) and distributed execution groups (`sched_deg.c`) exist. However, true strict preemption controls and bounded critical sections needed for safety profiles are missing robust architectural enforcement (largely relying on basic interrupts).

### Interrupt / Timer / IPI Stack
* **Status:** Stub / Placeholder
* **Details:** `interrupt_common.c` and `timer_common.c` exist in `kernel/src/hal/`, but they are very small (few lines) and function mostly as interfaces. Real hardware drivers (e.g., APIC, GICv3) are poorly implemented or stubbed. Clock source (monotonic) and clock event splitting are weak.

### IPC / Multikernel Messaging
* **Status:** Production-ready (mostly)
* **Details:** The `ipc` subsystem is highly developed. URPC (`lib/urpc`) provides high-throughput lockless ring-buffers for async shared memory, and cross-core multikernel messages route through `mk_dispatch.c` with validation. Deterministic endpoint IPC (`endpoint_ipc.c`, `async_ipc.c`) is implemented nicely.

### Security / Isolation / Capabilities
* **Status:** Implemented but incomplete
* **Details:** The VFS capability model (`vfs_mount_t`, `vfs_file_t`) and the general object capability engine (`kernel/src/capability.c`, `security/isolation.c`, `policy.c`) are well underway. Secure boot attestation (`secure_boot.c`) is plumbed, but raw signature verification happens at a basic level or defers to firmware rather than robust OS integration for all target profiles.

### Driver Model / DMA / IOMMU
* **Status:** Stub / Placeholder
* **Details:** IOMMU (`iommu_stub.c`) and VFIO (`vfio.c`) are heavily stubbed. The core driver model (`kernel/src/device/`) has basic probing but lacks deterministic init ordering for safety buses (CAN/SPI/I2C/UART).

### Storage / Filesystem
* **Status:** Stub / Placeholder (Architecturally needs rework)
* **Details:** `kernel/src/fs/vfs.c` is minimal string-matching logic for paths and drivers. The VFS layer relies purely on stubs. It lacks the lightweight compact inode/path caches necessary for the Edge/IoT profiles.

### Networking
* **Status:** Not implemented
* **Details:** Aside from some basic ethernet hooks inside the automotive personality and a `ptp_clock.c` stub, a real, lightweight embedded TCP/UDP stack or zero-copy packet path is missing from the kernel/core.

### GUI / Console / Framebuffer
* **Status:** Stub / Placeholder
* **Details:** A basic `fb_console.c` exists. There is no true compositor-friendly handoff or low-footprint UI stack suitable for Edge or Desktop profiles.

### Observability / Watchdog / Recovery
* **Status:** Implemented but incomplete
* **Details:** The automotive personality (`subsys/automotive.c`) has robust watchdog array and domain health tracking. Core kernel panic recovery (`panic.c`) has a toggle but is not a deeply resilient synchronous fault containment domain.

### Personality Support
* **Status:** Implemented but incomplete
* **Details:** `subsys/linux/linux_compat.c` exists for desktop/server. The Android and Automotive layers are largely API wrappers (e.g., `automotive.c` is ~500 lines of queues, watchdogs, and hooks) but miss full hardware linkage.

---

## 2. Target Deployment Classes Evaluation

### Small Device / Edge
* **Usable Subsystems:** Core Multikernel IPC (URPC), Capability-based Isolation, Scheduler (basic Round-Robin/Priority).
* **Missing for Production:** Lightweight VFS (currently heavily stubbed), Network stack (non-existent), robust Power Management (tick suppression/autosuspend), Secure Update (OTA) hooks.
* **Highest Priority:** Implementing a real lightweight VFS and a baseline TCP/UDP network stack so the device can communicate and store simple configs.

### Drone (RTOS / Robotics)
* **Usable Subsystems:** Synchronous IPC, deterministic priority scheduler, basic profile toggles.
* **Missing for Production:** Real Hardware Timers/Interrupts (too stubbed), Deterministic Safety Bus Drivers (CAN/SPI/I2C init ordering), Static Memory Pools (currently relying on generic PMM/slabs), synchronous fault containment.
* **Highest Priority:** Splitting HAL timers into monotonic vs deadline (one-shot per core) properly and building real deterministic I2C/SPI drivers for flight controllers/sensors.

### Automobile (Automotive ECU/Infotainment)
* **Usable Subsystems:** `subsys/automotive.c` provides a good software model for bounded deterministic queues, watchdog domain health, and boot stage planning.
* **Missing for Production:** Genuine underlying CAN/LIN/Ethernet drivers (the `subsys_automotive_send_can_frame` merely returns true), IOMMU/VFIO protection for isolating untrusted devices (IOMMU is a stub), strong Crash Consistency.
* **Highest Priority:** IOMMU implementations (VT-d/SMMU) and real CAN driver integration with the deterministic queues.

---

## 3. Concrete Improvement Plan

### Top 10 Gaps
1. **Hardware Timers & Interrupts (Arch):** Stubbed HAL components need real GICv3/APIC and one-shot deadline timers.
2. **IOMMU / DMA Isolation (Arch/Core):** `iommu_stub.c` must be replaced with real ACPI/FDT discovered backends to enable untrusted device blocking.
3. **Lightweight VFS (Core):** `vfs.c` is just a string matcher; it needs real compact inode cache and layer 2/3 storage bindings.
4. **Networking Stack (Core/Services):** No TCP/UDP stack exists for Edge/Appliance targets.
5. **Static Memory Pools (Core):** PMM/Slab needs strict, pre-allocated static pool paths with no fallback to heap for RTOS/Drone.
6. **Safety Bus Drivers (Core/Arch):** Deterministic initialization and real polling/interrupt drivers for CAN/SPI/I2C.
7. **Strict Preemption / Critical Sections (Core):** The scheduler needs hard boundaries for deterministic RT execution.
8. **Watchdog Hardware Linkage (Arch/Personality):** The automotive software watchdog logic must map to a real hardware watchdog (e.g., `rtos_watchdog_pet()`).
9. **Power Management / Suspend (Core):** Tick suppression and idle autosuspend are missing for Mobile/Edge.
10. **Secure OTA Updates (Core/Services):** Measured boot has stubs, but rollback/update orchestration is entirely missing.

### Recommended Implementation Order
1. **Phase 1 (Foundation):** Fix HAL timers, interrupts, and IOMMU stubs. (Gaps 1 & 2). *Without real time and isolation, safety features are theater.*
2. **Phase 2 (RTOS/Drone core):** Implement Static Memory Pools, Strict Preemption, and real Safety Bus (SPI/I2C/CAN) drivers. (Gaps 5, 6, 7).
3. **Phase 3 (Edge/Automotive):** Connect hardware watchdogs to the automotive subsystem, implement Lightweight VFS, and basic Network stack. (Gaps 3, 4, 8).
4. **Phase 4 (Refinement):** Power management and Secure OTA hooks. (Gaps 9, 10).

### Cross-Profile vs Profile-Specific
* **Cross-Profile:** IOMMU/DMA, HAL Timers/Interrupts, VFS expansion, Networking.
* **Profile-Specific:** Static Memory Pools (RTOS/Drone), Watchdog hardware integration (Automotive/Drone), Power Management (Mobile/Edge).

### Architectural Division (Where to implement)
* **Core Kernel (`kernel/src/`):** VFS core logic, Scheduler preemption constraints, Static memory pool API, Networking API.
* **Architecture Specific (`kernel/src/hal/`):** Timers (APIC/GIC/PLIC), Interrupt routing, IOMMU hardware interaction, Hardware watchdog triggers.
* **Personality Layer (`subsys/`):** Linux POSIX path resolution mappings, Automotive health state machine (already largely there), Android Binder routing.
