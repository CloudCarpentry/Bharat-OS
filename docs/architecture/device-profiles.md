# Device Profiles and Subsystem Matrix

This document summarizes how Bharat-OS maps a common microkernel core to different device categories and deployment profiles.

## The Scaling Philosophy

For edge devices, the right move is **not** to "shrink everything everywhere." It is to make Bharat-OS a **tiny stable kernel + profile-selected services + hard feature gates**. The refactor goal is to **make one kernel codebase scale down cleanly**, while accepting that the smallest MCUs may run only a subset or a paired control runtime.

For the hardware range:
* **AM6x / ARM64 / 1–4 GB**: full Bharat-OS profile is realistic.
* **AM2x / Cortex-R / a few MB RAM**: compact edge/safety profile only.
* **Hercules / 128–512 KB RAM**: not full Bharat-OS as-is; this needs a **micro-profile or companion RT executive**.
* **C2000**: partial only; better as a co-processor target, not the main Bharat-OS host.

## Three Strict Layers

To avoid blurred boundaries, the kernel codebase is split into three strict layers:

* **Tier A: Always-on microkernel core**
  * trap/syscall
  * scheduler primitives
  * IPC/uRPC primitives
  * capability checks
  * minimal VM/MMU hooks
  * interrupt/timer core
  * fault delivery
* **Tier B: Optional kernel facilities**
  * advanced VM
  * DMA/IOMMU helpers
  * SMP/multikernel coordination
  * telemetry hooks
  * power hooks
* **Tier C: Services/stacks only (User-space)**
  * networking policy
  * storage policy
  * crypto policy
  * device orchestration
  * update manager
  * UI/media

## Profile Model: Real Kernel Profile Descriptor

Size control is no longer managed through scattered `#ifdef`s. Bharat-OS uses one explicit profile object (`kernel_profile_t`) selected at build and validated at boot:

* `PROFILE_EDGE_MIN`
* `PROFILE_EDGE_SAFE`
* `PROFILE_AUTOMOTIVE`
* `PROFILE_GENERAL`
* `PROFILE_CLOUD`

Each profile defines bounds on the system, such as max cores, virtual memory mode, capability table size, IPC queue counts, tracing level, fault policy, required services, and optional services.

## Subsystem Registry

Every optional subsystem must self-register and be droppable. The subsystem registry tracks:
* Name
* Init level
* Profile mask
* Hard dependencies
* Memory footprint estimate
* Boot-critical flag

The boot path enables only what the selected profile needs, preventing dead code linking in compact edge builds.

## Practical Build Strategy (Deliverables)

Bharat-OS targets three explicit deliverables:

### Bharat-OS Nano
For the smallest feasible boards.
* single-core
* static memory pools
* MPU/MMU-lite
* minimal IPC
* no advanced VM
* no rich networking
* no media

### Bharat-OS Edge
For AM2x / mid-edge.
* bounded multiservice architecture
* compact networking
* telemetry
* watchdog
* power hooks
* fault domains

### Bharat-OS Full
For AM6x and above.
* SMP
* fuller VM
* richer networking/storage
* broader service graph
* display/media optional

## Implementation Roadmap (The 8 Engineering Tasks)

The following 8 tasks form the core engineering plan to realize the Nano/Edge/Full scaling model.

### 1. Add `kernel_profile_t` and build-time profile selection
* **Goal**: Replace `#ifdef` scatter with a structured profile descriptor.
* **High-Level Task**: Define `kernel_profile_t` and enums (`PROFILE_EDGE_MIN`, etc.). Update the build system to generate a specific profile header.
* **Low-Level Code Map**:
  * Modify `core/kernel/include/profile/profile.h` to define the new profile enums instead of relying on legacy `KernelExecutionProfile`.
  * Add the profile bounds structure (max cores, capability table size, trace level, etc.) to a new or existing header, e.g., `core/kernel/include/profile/kernel_profile.h`.
  * Update `core/kernel/src/kernel_boot.c` to validate the loaded profile object early in `boot_common_early()`.

### 2. Add subsystem registry with profile filtering
* **Goal**: Enable optional subsystems to self-register and be conditionally dropped based on the active profile.
* **High-Level Task**: Create a `subsystem_registry_t` mechanism. Let subsystems define their init level, mask, and dependencies.
* **Low-Level Code Map**:
  * Update `core/kernel/include/subsystem_profile.h` to introduce `subsystem_registry_t` with macro-based self-registration (similar to module init or linker-set arrays).
  * Refactor `bharat_subsystems_init()` in `core/kernel/src/kernel_boot.c` or a dedicated registry implementation file to iterate over the registry and only initialize subsystems that match the active profile mask.

### 3. Split MM into minimal core and advanced VM module
* **Goal**: Keep only fixed page allocation, minimal address space abstraction, map/unmap/protect, and fault basics in the core. Move large page sophistication, migration, complex reclaim, and debug-heavy walkers to advanced VM.
* **High-Level Task**: Refactor the `mm` subsystem.
* **Low-Level Code Map**:
  * `core/kernel/src/mm/pmm/pmm.c` and `core/kernel/include/mm/pmm.h`: Audit and separate core allocations from NUMA/migration logic.
  * Move advanced features out of `kernel.elf`'s core link list for `Nano` builds. Ensure the build system (`core/kernel/CMakeLists.txt`) can conditionally exclude `bharat_mm_vmm` features based on the profile.

### 4. Add allocation class tags
* **Goal**: Support swapping allocator backends per profile without touching callers.
* **High-Level Task**: Introduce `MEM_NORMAL`, `MEM_RT`, `MEM_DMA`, `MEM_SECURE`, `MEM_PACKET` classes.
* **Low-Level Code Map**:
  * Add an `alloc_class_t` enum to `core/kernel/include/mm/pmm.h` or a dedicated `core/kernel/include/mm/alloc_class.h`.
  * Update `pmm_alloc_pages()` or wrapper macros to accept the allocation class instead of relying purely on legacy zones (`PMM_ZONE_NORMAL`, etc.).

### 5. Convert core kernel objects to bounded pools for compact profiles
* **Goal**: Eliminate unbounded linked-list growth in hot paths and hidden heap dependencies.
* **High-Level Task**: Implement static object pools for threads, endpoints, capabilities, timers, and IRQ descriptors when a compact profile is selected.
* **Low-Level Code Map**:
  * Update `core/kernel/include/sched/sched.h` and `core/kernel/src/sched/` to use array-based bounded thread pools instead of heap allocations when `#if PROFILE_IS_COMPACT`.
  * Update `core/kernel/include/ipc/` and capability management (`core/kernel/src/cap/`) to pre-allocate slots based on the bounds defined in the active `kernel_profile_t`.

### 6. Add fault domains + restart metadata
* **Goal**: Support safe restart of device managers or network managers without rebooting the whole system.
* **High-Level Task**: Add fault domains, restart policies, safe-mode markers, and fault reason codes to process/service control blocks.
* **Low-Level Code Map**:
  * Define `fault_domain_t` and `restart_policy_t` in `core/kernel/include/core/kernel/kernel_safety.h` or `core/kernel/include/sched/fault_domain.h`.
  * Augment the scheduler process descriptor (`bh_process_t` or equivalent in `core/kernel/src/sched/`) to track these metadata fields.
  * Update the fault handler (`core/kernel/src/trap/` and `core/kernel/src/mm/vm/fault/fault.c`) to query the fault domain and trigger partial restarts via IPC instead of global panics.

### 7. Standardize IPC/uRPC endpoint descriptor and message classes
* **Goal**: Standardize IPC classes to prevent bloat and maintain a compact endpoint descriptor.
* **High-Level Task**: Define explicit standard classes: control, dataplane, telemetry, safety, storage, power. Keep one compact endpoint structure.
* **Low-Level Code Map**:
  * Update `core/kernel/include/ipc/ipc_endpoint.h` and `core/kernel/include/urpc/` message envelopes to include an explicit `msg_class` enum.
  * Refactor existing IPC paths to validate against these classes. Clean up any ad-hoc or duplicated messaging structures across `core/services/`.

### 8. Introduce class-based device registration and move policy to services
* **Goal**: The kernel should only know device class, IRQ binding, MMIO resource, DMA constraints, and power state hooks. All policy (routing, thermal, update, storage mounts) moves to user space.
* **High-Level Task**: Standardize the device framework around generic classes.
* **Low-Level Code Map**:
  * Enhance `core/kernel/include/device.h`. Ensure `device_driver_t` and `device_desc_t` do not contain subsystem-specific operational policy.
  * Sweep `core/kernel/src/device/` (or related board files) to ensure no routing or thermal policy is hardcoded.
  * Enforce that actual core/drivers/policy live in `core/services/core/`, `core/services/system/`, `core/services/device/`, etc. (e.g., thermal throttling belongs in `core/services/device/thermal_mgr`).
