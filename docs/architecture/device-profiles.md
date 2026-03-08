# Device Profiles and Subsystem Matrix

This document summarizes how Bharat-OS maps a common microkernel core to different device categories and deployment profiles.

## Profile model

Bharat-OS keeps the kernel core small and portable, then composes profile-specific capability packs in user space.

### Shared kernel core (all profiles)

- boot and platform initialization
- scheduler and timer/interrupt framework
- memory map/unmap primitives
- capability/object/IPC substrate
- SMP bring-up and per-core scaffolding
- driver and syscall boundary primitives
- observability, tracing, and crash hooks

## Bharat-RT vs Bharat-Cloud

| Area | Bharat-RT | Bharat-Cloud |
| --- | --- | --- |
| Memory policy | static allocation; paging disabled for deterministic bounds | demand paging with user-space pagers |
| Scheduling target | bounded latency and low jitter | throughput, fairness, and core utilization |
| Core topology | dedicated cores for mixed-criticality partitions | many-core and NUMA-aware placement |
| IPC emphasis | low-latency synchronous endpoint paths | lockless URPC for cross-core/service scaling |
| Typical devices | embedded/mobile/robotics/UAV | server, appliance, cloud, AI clusters |

## Device categories and subsystem emphasis

### Mobile and embedded

- capability-gated sensor/peripheral access
- user-space driver restartability for resilience
- power/thermal tuning and suspend/resume hooks
- framebuffer-first UX for low-cost panels; compositor optional

### Edge and IoT gateways

- minimal attack surface with service sandboxing
- watchdog/recovery and secure OTA paths
- industrial networking and telemetry-first operations
- headless-first, with optional local framebuffer dashboard

### Robotics and autonomous systems

- mixed-criticality partitioning via capabilities
- static-allocation real-time profile support
- dedicated-core pipelines (sensor fusion, planning, control)
- deterministic IPC across tasks and cores

### Drones and UAVs

- flight-control determinism and failsafe state machines
- tight power/thermal budgets and telemetry prioritization
- strict isolation between flight-critical and auxiliary services
- low-overhead cross-core messaging for sensor/navigation workloads

### Network appliances

- user-space network drivers with isolation boundaries
- fast-path packet processing and QoS shaping hooks
- service restartability without full-kernel panic
- control-plane hardening via capability-mediated access

### Datacenter and cloud

- multikernel scaling across many-core hosts
- NUMA-aware memory and scheduling policy
- accelerator-aware placement and service partitioning
- distributed-memory roadmap including CXL-oriented design paths

## Multikernel and research alignment

Bharat-OS adopts a multikernel direction inspired by Barrelfish: treat cores as cooperative nodes using explicit message passing instead of shared-kernel lock contention. This aligns with the architecture goal of predictable scaling as core counts and heterogeneity rise.
