# Profile-first roadmap: kernel/core impact and subsystem deltas

This document maps each hardware/deployment profile to:

1. What to build first.
2. Core-kernel changes needed.
3. Subsystems that require extensions or refactors.

## Cross-profile kernel baseline (do once)

Before tuning per profile, the core kernel should expose stable contracts for:

- **Boot policy selection** (security/perf/timer/SMP toggles).
- **Scheduler classes** (deterministic RT, fair, throughput).
- **IPC primitives** (endpoint, shared-memory, async message path).
- **Memory classes** (static pools, pageable heaps, NUMA domains).
- **Device model** (driver probe + policy hooks).
- **Observability hooks** (tracepoints, counters, crash logs).

These are the profile control points that avoid code forks.

---

## RTOS / safety profile

### Build first
- interrupt/timer
- deterministic scheduler
- lightweight IPC
- static memory pools
- watchdog
- CAN/UART/SPI/I2C
- fast boot
- health monitor

### Core kernel changes
- Add strict preemption controls and bounded critical sections in scheduler core.
- Prefer static allocation paths; avoid runtime heap dependency in early/critical paths.
- Enforce synchronous fault containment (panic domain + watchdog-safe reset path).
- Reduce boot variance by disabling optional non-critical bring-up (e.g., delayed services).

### Subsystems needing changes
- **HAL**: interrupt latency accounting, periodic timer precision, watchdog feed semantics.
- **MM**: fixed-size memory pools and deterministic alloc/free timing.
- **IPC**: bounded-copy endpoint path with timeout guarantees.
- **Drivers**: deterministic init ordering for safety buses (CAN/SPI/I2C/UART).
- **Health**: liveness checks tied to watchdog window.

---

## Edge / IoT profile

### Build first
- lightweight VFS
- network stack
- secure boot hooks
- OTA/update subsystem
- framebuffer/simple UI
- shared memory IPC
- power management

### Core kernel changes
- Keep small memory footprint with configurable VFS + networking feature slices.
- Introduce secure update and rollback control points into boot chain.
- Add low-power idle states and wake policy integration into scheduler/timer.

### Subsystems needing changes
- **VFS**: compact inode/path cache policies and read-mostly optimizations.
- **Net**: small-footprint TCP/UDP path with predictable buffer limits.
- **Security**: measured boot + update signature verification plumbing.
- **Power**: tick suppression and peripheral autosuspend.

---

## Desktop / laptop profile

### Build first
- full VFS
- ELF loader
- Linux personality baseline
- tty/pty
- input
- framebuffer + display stack
- audio later
- network
- storage
- power/thermal

### Core kernel changes
- Expand process/fd/memory semantics for POSIX/Linux compatibility layers.
- Grow driver model toward interactive devices and storage hierarchy.
- Add robust user-session I/O pathways (tty/input/display).

### Subsystems needing changes
- **Process/Trap**: richer syscall surface and signal-compatible semantics.
- **VFS**: full pathname/permission/link semantics.
- **Display/Input**: compositor-friendly framebuffer handoff and event routing.
- **Storage/Net**: desktop-class buffering and fault tolerance.

---

## Mobile / Android profile

### Build first
- Linux-like process/fd/memory substrate
- Binder-oriented IPC hooks
- ashmem/shared memory
- service manager
- power management
- input/display pipeline
- security hooks
- app/runtime spawning hooks

### Core kernel changes
- Add Binder/ashmem integration points into IPC + memory subsystems.
- Strengthen per-app isolation and credential transitions.
- Optimize suspend/resume and foreground/background scheduling policy.

### Subsystems needing changes
- **IPC**: Binder transaction model and object reference lifecycle hooks.
- **MM**: ashmem-like shared regions and reclaim policy coordination.
- **Security**: mandatory hooks for app sandbox + service mediation.
- **Lifecycle**: app spawn/zygote-like runtime interfaces.

---

## Datacenter / server profile

### Build first
- NUMA-aware memory
- scalable scheduler
- high-performance network
- block I/O
- observability
- namespaces/cgroups later
- strong crash recovery
- virtio + VM support

### Core kernel changes
- Make scheduler topology-aware and scalable to high core counts.
- Expand NUMA placement/migration policy in allocator and VM mappings.
- Prioritize crash consistency and restart/recovery strategy.

### Subsystems needing changes
- **MM/NUMA**: locality-aware allocations, migration telemetry.
- **Scheduler**: per-core run queues, load balancing heuristics.
- **Net/Block**: high-throughput queueing, multiqueue IO paths.
- **Observability**: structured metrics + trace export.
- **Virtualization**: virtio and VM lifecycle hooks.

---

## Network appliance profile

### Build first
- packet path
- NIC drivers
- flow tables
- lightweight control plane IPC
- watchdog/failover
- secure boot
- traffic shaping/QoS hooks

### Core kernel changes
- Introduce fastpath bypass where safe (minimal scheduler/allocator overhead).
- Support flow-table acceleration hooks and deterministic control-plane messaging.
- Implement failover-aware health and restart orchestration.

### Subsystems needing changes
- **Net**: fast ingress/egress, zero-copy rings where possible.
- **Drivers**: NIC offload capability abstraction.
- **IPC**: low-latency control channel for routing/policy updates.
- **QoS**: classifier + shaper hooks tied to scheduler/network queues.
- **Security/HA**: secure boot attestation + crash/failover policy.

---

## Immediate implementation status in this repo

Current repository changes focus on **profile plumbing** in core kernel:

- Build system accepts and routes `mobile`, `datacenter`, and `network_appliance` boot profiles.
- Boot profile detection string support is added in kernel main.
- Boot policy defaults are added for these profiles (timer/SMP/security/perf knobs).
- New per-profile init stubs are added so each profile has a concrete source target.

This establishes the foundation for next patches that implement deeper subsystem behavior.
