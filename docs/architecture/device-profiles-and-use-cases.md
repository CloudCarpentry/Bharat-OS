---
title: Device Profiles and Use-cases
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
see_also:
  - README.md
---
# Device Profiles and Use-cases

## Purpose

This document maps Bharat-OS architectural features to practical deployment classes and distinguishes **implemented baseline** vs **roadmap**.

## Profile Composition Model

Bharat-OS uses profile-oriented tuning to keep a shared kernel spine while changing policy and scale assumptions. The architecture specifically scales down through formal profiles rather than a messy half-port.

The three primary delivery tiers are:

- **Bharat-OS Nano (Micro-profile / RTOS):** Smallest footprint and bounded behavior. Designed for Hercules/C2000 class where full OS features are not viable. Single-core, static memory pools, MPU/MMU-lite, minimal IPC.
  - **Constraints**: 128KB - 512KB RAM, <512KB Flash.
- **Bharat-OS Edge (Mid-edge):** Constrained power/thermal envelopes with mixed criticality workloads. Designed for AM2x/Cortex-R class. Includes bounded multiservice architecture, telemetry, watchdog, power hooks, and fault domains.
  - **Constraints**: 1MB - 2MB RAM, <1MB Flash.
- **Bharat-OS Full (Desktop/Server/Cloud):** Broader throughput and service composition goals. Designed for AM6x and above. Includes SMP, full VM, rich networking/storage, and broad service graphs.

For a deep dive into achieving these small footprints, see the [Low-Footprint Design and Analysis](low-footprint-design.md) document.

## Existing / General Profiles

### Mobile / Wearables (MOBILE)
**Current fit**
- Capability isolation helps reduce blast radius for compromised components.
- Scheduler telemetry hooks + AI suggestion path can support power-aware policy in user space.

**Gaps / roadmap**
- Production DVFS/thermal control integration and power-model calibration.
- Hardening for sustained battery-constrained runtime behavior.

### Robotics / Drones (ROBOT, DRONE)
**Current fit**
- Message-passing architecture and bounded queues support deterministic control-plane behavior.
- Architecture portability (x86_64/riscv64/arm64 build surfaces) supports mixed platform development.

**Gaps / roadmap**
- Stronger real-time scheduling guarantees and admission control.
- More complete fault containment and recovery semantics for safety-critical subsystems via explicit fault domains and restart metadata.

### Network appliances / Edge gateways (EDGE, IOT)
**Current fit**
- Capability/driver boundary model supports service isolation.
- Baseline multikernel messaging is compatible with control/data plane separation patterns.

**Gaps / roadmap**
- Mature high-throughput network stack/data plane acceleration.
- Better asynchronous eventing and device-interrupt integration depth.
- Standardized IPC/uRPC message classes (control, dataplane, telemetry) to cut code duplication.

### Laptop / Desktop (LAPTOP, DESKTOP)
**Current fit**
- Full VM support and SMP scalability allow for rich multitasking environments.
- Capability-mediated driver access provides strong security for consumer devices.

**Gaps / roadmap**
- Advanced power management for mobile form factors.
- Full desktop compositor and UI subsystem integration.

### Medical Devices (MEDICAL)
**Current fit**
- Strict isolation and capability-based security are ideal for certified medical hardware.
- Formal profile-driven composition ensures only necessary services are included, reducing the attack surface.

**Gaps / roadmap**
- Formal verification of core safety-critical paths.
- Hardware-specific qualification for medical sensor interfaces.

### Data-center / Clustered services
**Current fit**
- Multicore + NUMA scaffolding and URPC channels establish scale-out direction.
- Policy/mechanism separation (including AI policy boundary) supports operational control planes.

**Gaps / roadmap**
- Full distributed scheduler behavior across nodes.
- Production-grade memory management depth and service lifecycle orchestration.

### Research and Education Platforms
**Current fit**
- Clear separation of baseline vs deferred components supports experimentation.
- ADR-driven governance provides traceability for design decisions.

## Safety-Oriented and Vehicle Profiles

Bharat-OS targets several safety-critical domains where architectural isolation and deterministic behavior are paramount.

### ADAS / Autonomous Driving

Bharat-OS is a good fit for future ADAS and autonomous-driving control platforms because the architecture separates safety-critical control paths from policy-heavy user-space services.

**Potential value:**
- **Mixed-criticality partitioning:** perception, planning, control, telemetry, diagnostics, and UI can run in isolated domains with explicit capabilities.
- **Deterministic IPC/URPC:** low-latency message paths can connect camera/radar/lidar preprocessing, fusion, planning, and actuator-control services without making the kernel policy-heavy.
- **Restartable user-space drivers:** sensor or accelerator drivers can fail and restart without forcing whole-system failure.

**Example mapping:**

| ADAS function | Bharat-OS mapping |
| --- | --- |
| Camera/radar/lidar input | User-space sensor drivers + capability-gated DMA/MMIO |
| Perception pipeline | Accelerator/NPU/GPU service domain |
| Sensor fusion | RT or latency-sensitive service domain |
| Planning/control | RT profile with deadline-aware scheduling |
| Vehicle communication | CAN/Ethernet gateway service |
| Diagnostics | Telemetry + fault-domain service |
| Safe fallback | Safety manager + watchdog + safe-state transition |

### Gateway ECUs

Bharat-OS can target gateway ECUs because its architecture naturally separates network-facing, vehicle-bus-facing, and diagnostic domains.

**Potential value:**
- **Capability-mediated networking:** each network interface, bus, or diagnostic endpoint can expose only the rights required by that service.
- **Gateway service isolation:** routing, filtering, diagnostics, OTA, and telemetry can be separate services rather than one privileged monolith.
- **Restartable communication stacks:** CAN, Ethernet, SOME/IP-like services, diagnostics, and update channels can restart independently.

**Example mapping:**

| Gateway ECU function | Bharat-OS mapping |
| --- | --- |
| CAN/CAN-FD bus access | Vehicle stack + bus driver capability |
| Automotive Ethernet | Network stack + netmgr control plane |
| Diagnostics | Isolated diagnostic service |
| Firewall/routing | User-space gateway policy service |
| OTA/update | Secure update service roadmap |
| Fault reporting | Telemetry/diagnostic event service |

### Industrial Control

For industrial control, Bharat-OS should focus on deterministic timing, strong isolation, and resilient service recovery.

**Potential value:**
- **RT-leaning profile:** deterministic memory allocation, deadline metadata, and bounded IPC paths can support PLC-like and motion-control workloads.
- **Sensor/actuator isolation:** actuator control can be capability-gated and separated from HMI, logging, and remote-management services.
- **Fault containment:** a failed HMI, logging service, or network service should not directly corrupt control-loop execution.

**Example mapping:**

| Industrial function | Bharat-OS mapping |
| --- | --- |
| Control loop | RT service with deadline metadata |
| Sensor sampling | Sensor manager + timestamped ring buffer |
| Actuator command | Actuator queue + safe-stop path |
| HMI panel | Framebuffer/lightweight UI profile |
| Remote monitoring | Network + telemetry service |
| Safety interlock | Watchdog + safety manager |

### Aerospace Systems

Aerospace use cases require extreme caution, but Bharat-OS has architectural ingredients that are useful for future research and prototyping.

**Potential value:**
- **Minimal trusted kernel base:** only scheduling, memory, traps, capabilities, IPC/uRPC, and fault handling should remain in kernel.
- **Strict domain separation:** navigation, telemetry, payload, communications, and diagnostics can run as isolated services.
- **Fault-domain model:** service failure can be isolated and escalated to safe-mode policy instead of uncontrolled system-wide failure.

**Example mapping:**

| Aerospace function | Bharat-OS mapping |
| --- | --- |
| Flight-critical loop | RT/safety profile service |
| Telemetry | Isolated telemetry service |
| Payload control | Separate capability-bounded service |
| Communication bus | Network/radio stack service |
| Health monitoring | Fault-domain + watchdog framework |
| Safe mode | Safety manager policy service |

### Digital Cockpit / Infotainment

Digital cockpit is different from ADAS: it needs UI, media, Android/Linux compatibility paths, and strong separation from vehicle-control domains.

**Potential value:**
- **Domain split:** infotainment, cluster, navigation, voice assistant, media, and vehicle-status display can be separated.
- **Display tiers:** headless, text console, framebuffer, lightweight embedded UI, and full compositor can be enabled by profile.
- **Multi-personality direction:** Linux/Android personality work can support app/runtime compatibility without moving those compatibility layers into the kernel.

**Example mapping:**

| Cockpit function | Bharat-OS mapping |
| --- | --- |
| Instrument cluster | Lightweight UI/framebuffer service |
| Infotainment | Media stack + Android/Linux personality path |
| Navigation | App/service domain |
| Vehicle status | Read-only vehicle data capability |
| Voice/AI assistant | User-space AI/accelerator service |
| Secure warning overlay | Trusted UI/display broker roadmap |

## Cross-Domain Primitive Matrix

| Primitive | ADAS | Gateway ECU | Industrial | Aerospace | Digital Cockpit |
| --- | --- | --- | --- | --- | --- |
| Capability isolation | required | required | required | required | required |
| Fault domains | required | required | required | required | recommended |
| Deadline/timer API | required | recommended | required | required | recommended |
| Device class registry | required | required | required | required | required |
| Telemetry/diagnostics | required | required | required | required | required |
| Watchdog/safe-state | required | recommended | required | required | recommended |
| Display stack | optional | optional | HMI only | optional | required |
| Vehicle/sensor stack | required | required | actuator/sensor | mission bus | vehicle data read-only |

## Current Maturity Gaps

Before claiming production readiness for safety-oriented profiles, Bharat-OS must close these gaps:
- Bounded scheduler and cross-core migration behavior.
- Bounded TLB shootdown and memory invalidation protocol.
- Strict capability enforcement at every IPC/service boundary.
- Real service supervisor with restart/backoff/watchdog policy.
- Fault-domain and safe-state transition framework.
- Deadline/timer primitives with tests.
- Device-class registry for sensors, actuators, display, network, storage, and accelerator devices.
- Telemetry and diagnostic event service.

## Next Implementation Roadmap

1. **Implement fault-domain primitives** and service-facing fault event contract.
2. **Develop device-class registry** for standardized sensor and actuator access.
3. **Formalize watchdog and safe-state metadata** as a kernel/service contract.
4. **Close capability enforcement gaps** in all core manager paths.

## References and alignment

- Multikernel direction aligns conceptually with Barrelfish’s messaging-first multicore argument.
- Capability discipline aligns conceptually with seL4/L4-family authority modeling.

These are architectural influences rather than claims of implementation equivalence.
