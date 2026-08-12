# Bharat-OS Production-Grade Readiness Analysis Report

This report analyzes the core subsystems of Bharat-OS to identify tasks necessary for transitioning the operating system to a production-grade status. The analysis covers memory modeling, scheduling, boot handoffs, process and thread creation, architecture profiles, device discovery, and graphical user interfaces.

## 1. Memory Model

The Bharat-OS memory model employs a foundational slab allocator (`slab.c`) and a virtual memory allocator (`kvmalloc`), alongside a standard buddy allocator.

**Production Readiness Tasks:**
- **Advanced Slab Allocators:** Implement `SLUB` (for SMP scalability and debuggability) and `SLOB` (for highly constrained edge devices) variants to replace the basic `kcache_t` SLAB allocator depending on system profile requirements.
- **NUMA Support:** The `hal_topology_init` implementation provides fallback UMA support but relies on bootloader metadata for NUMA detection. Ensure robust demand-paging support (`mm_remote.c` and `vm_compat_vmm.c`) takes NUMA affinity into account.
- **Capability Mediation Verification:** Continuous audit required to ensure full capability-based memory protection without global mutable state.

## 2. Scheduler

The Bharat-OS scheduler supports RT (Real-Time) profiles (`PROFILE_KERNEL_RT`) and MIX (Mixed-Criticality) profiles. Partitioning is handled by `cpu_partition.c` which parses rules for `BHARAT_SCHED_CLASS_SYSTEM`, `BHARAT_SCHED_CLASS_FIFO_RT`, `BHARAT_SCHED_CLASS_DEADLINE_RT`, and `BHARAT_SCHED_CLASS_FAIR`.

**Production Readiness Tasks:**
- **Dynamic Configuration Definition:** Add infrastructure to define YAML-like dynamic rules for partition classes and migrations (e.g. system `[0]`, safety_rt `[1,2]`, general `[4,5,6]`). Currently, `cpu_partition_init` uses hardcoded deterministic bootstrap policies.
- **Preemption & Latency:** Ensure tickless operations and RT-specific interrupt-disable paths are bounded. Priority Inheritance (PI) validation is required across all system call entry points.
- **Cross-Core State:** Move away from global locking paradigms towards per-CPU local states and strict lockless uRPC coordination.

## 3. Booting & Handoff

The bootstrap process relies on a minimal kernel acting as an early boot-stage supervisor (`init_bootstrap.c`), transitioning to userspace managers (`servicemgr`).

**Production Readiness Tasks:**
- **Lifecycle Migration:** Complete the architecture handoff from the trusted `init` bootstrap layer to unprivileged long-lived `servicemgr` and policy managers.
- **Robust Error Handling:** Enhance bootstrap paths to implement rollback and recovery mechanisms upon subsystem initialization failure, replacing basic console warnings.
- **Early Handoff Primitives:** Implement explicit primitives in the kernel to silence early framebuffer logging before handing control to the Wayland/display broker compositor.

## 4. Process & Thread Creation

Processes and threads are defined by contracts (e.g., BIDL) and managers like `process_manager.c`. Currently, `process_manager` is in a scaffold state.

**Production Readiness Tasks:**
- **Process Manager Realization:** Implement end-to-end ELF execution loading and dynamic capability resolution within `core/services/process_manager/`.
- **System Call ABI Freeze:** Finalize the canonical UAPI and syscall mappings (`native_syscalls.json` integration) and eliminate ad-hoc subsystem return codes.
- **Userspace IPC Wiring:** Implement actual BIDL client mappings to replace weak IPC stub linkages in standalone native apps.

## 5. SMP and AMP Architectures (GP, MIX, RT)

The system is designed for multi-core configurations (e.g., 8-core ARM64) supporting heterogeneous execution profiles: General Purpose (GP), Mixed (MIX), and Real-Time (RT).

**Production Readiness Tasks:**
- **Per-Core Ownership:** Strengthen the per-core data structures (runqueues, cap tables, memory pools) to ensure no mutable state crosses cores without uRPC coordination.
- **Hardware Profile Adaptation:** Implement a unified interface that translates hardware topologies (e.g., big.LITTLE, NUMA) directly into role configurations without hardcoded rules.

## 6. Device Discovery

Hardware device discovery utilizes runtime methods rather than static compilation, orchestrated via `devmgr` and `netmgr`.

**Production Readiness Tasks:**
- **FDT/ACPI Standardized Parsing:** Expand `fdt_parse_common.c` and ACPI bindings to securely ingest hardware capability maps from the bootloader/firmware into the `devmgr` service.
- **Unprivileged Driver Binding:** Transition in-kernel fallback drivers to unprivileged services that rely on the uniform Device Registry for capability delegation.

## 7. GUI & Early Boot Display

The early boot process can support both GUI and text-based setups via `boot_ui_select.c` and early framebuffer structures.

**Production Readiness Tasks:**
- **Capability-Gated Compositing:** Build a capability-based windowing/compositor architecture in `services/`. Ensure the framebuffer driver operates securely and handles pixel data securely.
- **Boot Handoff Coordination:** Ensure seamless transition between early kernel boot graphics (`boot_ui_select.c`) to standard userspace UI (like LVGL/Wayland displays connected via the `gui_showcase`) while preserving serial IO parity for text consoles.
- **Native Wayland Broker Integration:** Wire the standalone user-space display broker to consume `lvgl_display_adapter.c` and `lvgl_input_adapter.c` securely with no unmediated mapping overlaps.
