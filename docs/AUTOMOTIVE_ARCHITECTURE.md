# Bharat-OS Automotive & Mobility Architecture

## 1. Purpose

Bharat-OS targets **mixed-criticality mobility systems**:

- electric two-wheelers and three-wheelers
- passenger EVs
- robotics and drones
- industrial automation
- distributed ECU architectures

These environments require:

- fast boot (control readiness in <1-2s)
- hard real-time determinism for control loops
- strong safety and application fault isolation
- automotive/industrial communication support (CAN, TSN, SOME/IP)
- secure firmware lifecycle
- support for heterogeneous processors (ARM, RISC-V, SHAKTI, x86 for simulation)

Bharat-OS therefore adopts a **distributed kernel architecture with domain partitioning**. The system is organized as cooperating domains with explicit IPC contracts, rather than a single monolithic execution environment.

## 2. Design Principles

### Deterministic Control First

Control loops (motor torque, braking, BMS, watchdog) execute independently of service and AI workloads.

### Partitioned Execution

Domain isolation is required for mixed-criticality behavior:

- Safety Domain
- Control Domain
- Service Domain
- Application Domain (Linux companion)

### Distributed by Default

Vehicle deployments may use multiple ECUs (battery, motor, chassis, gateway, central compute), each running Bharat-OS components and communicating over deterministic transports.

### Hardware Agnostic, Hardware Aware

Core abstractions remain architecture-neutral while HAL surfaces timers, interrupts, DMA, and accelerator interfaces per target SoC.

## 3. Domain-Oriented System Architecture

```text
+------------------------------------------------------+
|                Application Domain                    |
| (Linux / AI / Infotainment / Cloud Services)        |
+------------------------------------------------------+
                |  Bharat Fabric IPC
+------------------------------------------------------+
|              Service Domain (Bharat-OS)             |
| Diagnostics | Storage | Gateway | Telemetry         |
+------------------------------------------------------+
                |  Deterministic IPC
+------------------------------------------------------+
|              Control Domain (Bharat-OS)             |
| Motor Control | BMS | Sensor Fusion | Timing        |
+------------------------------------------------------+
                |  Direct hardware access
+------------------------------------------------------+
|               Safety Domain (Bharat-OS)             |
| Watchdog | Health Monitor | Emergency Logic         |
+------------------------------------------------------+
                |  HAL
+------------------------------------------------------+
|                Hardware Platform                     |
| CPU | CAN | TSN | ADC | PWM | NPU | Flash           |
+------------------------------------------------------+
```

Domains can be mapped to separate cores, MPU partitions, or separate SoCs.

## 4. Kernel & Subsystem Responsibilities

### 4.1 Bharat Microcore (Privileged Minimal Kernel)

The microcore remains small and analyzable, responsible only for:

- scheduling primitives and dispatch
- interrupt routing and timer primitives
- memory protection setup
- IPC primitives and shared-memory channel setup
- domain lifecycle hooks (start/stop/restart)

### 4.2 Domain Runtime Layer

- **Safety Runtime**: watchdog chain, health monitoring, degraded-mode transitions, emergency policy execution.
- **Control Runtime**: hard real-time loops (motor control, BMS, actuator/sensor timing), fixed-priority scheduling.
- **Service Runtime**: diagnostics, logging, update agent, gateway routing, telemetry.
- **Linux Companion Runtime**: RPMsg/VirtIO-style integration and service boundary enforcement.

### 4.3 Core Kernel Subsystems Required for Automotive

- deterministic scheduler classes with CPU/core isolation
- capability-based IPC permissions and bounded queues
- monotonic and cross-core time synchronization
- safety-aware restart supervisor
- protocol abstraction layer for CAN/LIN/TSN/SOME-IP
- secure boot and signed update verification path

## 5. Boot Architecture

1. **Stage 1 - Boot ROM**: minimal hardware initialization and trusted boot handoff.
2. **Stage 2 - Platform Loader**: load microcore + safety + control domains first.
3. **Stage 3 - Service Domain**: diagnostics/storage/gateway services become available.
4. **Stage 4 - Application Domain**: Linux/AI/infotainment starts after control readiness.

Design rule: control and safety readiness is available before user-facing systems.

## 6. Bharat Fabric (IPC and Transport)

Bharat Fabric is the communication layer across domains/processors/ECUs.

Supported patterns:

- shared-memory lock-free queues
- mailbox interrupts
- request/response RPC
- pub/sub telemetry streams
- service discovery for distributed endpoints

Supported transports:

- shared memory
- mailbox registers
- RPMsg/VirtIO-compatible channels
- networked transports over CAN/TSN/Ethernet gateways

## 7. Automotive Communication Stack

- **CAN/CAN FD**: drivetrain, battery, actuator/sensor control buses
- **LIN**: low-speed peripherals and body electronics
- **Automotive Ethernet**: high-bandwidth cross-ECU links
- **TSN**: deterministic Ethernet timing and traffic classes
- **SOME/IP**: service-oriented discovery and control APIs

## 8. Scheduling, Time, and Determinism

Scheduler classes:

- **Hard RT Scheduler**: fixed-priority, bounded latency, non-preemptable critical sections with strict limits
- **Mixed-Criticality Scheduler**: CPU partitioning, runtime budgets, overload mode transitions
- **Service Scheduler**: best-effort background workloads

Time subsystem requirements:

- global monotonic time source
- cross-core timestamp normalization
- CAN frame timestamping support
- forward path for PTP/TSN clock discipline

## 9. Security and Safety Architecture

- secure boot with signed image validation
- measured boot chain for attestation and diagnostics
- strict domain memory/device ownership boundaries
- policy-controlled IPC rights
- signed firmware updates with rollback protection and staged rollout

## 10. Hardware Architecture Support

- **ARM**: Cortex-M/R/A profiles
- **RISC-V**: embedded and industrial controllers
- **SHAKTI**: ISA-compatible support via target-specific HAL integrations
- **x86_64**: development, simulation, CI, and QEMU-backed validation

## 11. Automotive Hardware Profiles

The following profiles define build-time and runtime policy bundles:

- `BHARAT_PROFILE_AUTOMOTIVE_SAFETY_RT`
- `BHARAT_PROFILE_AUTOMOTIVE_CONTROL`
- `BHARAT_PROFILE_AUTOMOTIVE_GATEWAY`
- `BHARAT_PROFILE_AUTOMOTIVE_AI_EDGE`

Each profile configures:

- scheduler class defaults and priority ceilings
- memory map and MPU/MMU boundaries
- protocol stack enablement (CAN/LIN/TSN/SOME/IP)
- driver allowlists and accelerator policy

## 12. Deployment Modes

- **Native Mode**: Bharat-OS directly on target hardware
- **Linux Companion Mode**: Linux and Bharat-OS partitioned by core/domain
- **Hypervisor Partition Mode**: mixed domains under a hypervisor layer
- **Distributed Multi-ECU Mode**: networked Bharat-OS instances across ECUs

## 13. Automotive Maturity Milestones & Roadmap

The development roadmap aligns with progressively rigorous automotive maturity expectations:

- **Milestone A**: bring-up and basic drivers
- **Milestone B**: deterministic comms and sensor timing
- **Milestone C**: safety supervision and diagnostics
- **Milestone D**: domain-controller/gateway readiness
- **Milestone E**: certification-oriented hardening

*Previous phased planning integrates directly into these maturity milestones.*

## 14. Target Applications

- electric mobility platforms
- autonomous robots and drones
- industrial automation
- smart mobility and gateway systems

## 15. Advanced Automotive Subsystems (The Control & Safety Plane)

Moving beyond generic OS components requires a dedicated focus on control-plane and safety-plane architectures.

### 15.1 Functional Safety Plumbing
This is the most critical layer for vehicle-grade operation:
* **Safety supervisor subsystem**: monitors heartbeats from critical services, checks deadline misses, and triggers degraded mode or safe-state transitions.
* **Watchdog framework**: per-core hardware watchdogs combined with service-level watchdog registration. Uses an escalation policy: restart service -> isolate subsystem -> enter limp-home mode.
* **Fault manager**: provides unified reporting for driver faults, sensor faults, bus faults, and timing faults. Classifies faults into severity classes: info, degraded, critical, immediate-safe-state.
* **Safe-state output path**: a direct, deterministic, non-IPC-dependent path for actuator neutralization upon critical failure.

### 15.2 AUTOSAR-like Structural Separation
While full AUTOSAR complexity is avoided, its structural separation is adopted:
* **MCAL-like Hardware Abstraction**: MCU and peripheral specifics remain fully isolated.
* **ECU Services & Communication Services**: central managers for protocols, routing, and vehicle state.
* **I/O Hardware Abstraction**: normalizes hardware signals into logical signals.
* **Application Personalities**: application logic remains strictly portable across boards; board files must never leak into vehicle control services.

### 15.3 Vehicle Power State & Wakeup Framework
Explicit management of ECU power states is mandatory:
* **State Model**: ignition, accessory, run, crank, off.
* **Sleep Preparation**: sleep preparation callbacks for active services.
* **Wake Source Framework**: GPIO wake, CAN wake, timer wake.
* **EV specific protections**: low-voltage battery protection logic, staged shutdown of non-critical domains, and wake authorization policy.

### 15.4 Time-Aware Scheduling and Timestamping
Automotive systems need absolute timing discipline to prevent control instability:
* **Global Monotonic Timestamp Service**: core API for all system time.
* **Hardware Timestamp API**: CAN RX/TX timestamps, GPIO capture timestamps, ADC conversion timestamps.
* **Timer Wheel / Time-Triggered Scheduler**: for deterministic periodic tasks.
* **Deadline Monitoring**: catching slipped control loop executions.

### 15.5 Diagnostics and DTC Subsystem
Essential for serviceability and fault tracing:
* **Diagnostic Trouble Code (DTC) Manager**: mapping services to DTCs.
* **Event Memory & Freeze-frame**: snapshot support for faults.
* **Debouncing Logic**: handling intermittent faults safely.
* **Fault Aging / Healing Logic**: auto-clearing non-persistent faults.
* **Service Hooks**: read/clear DTC capabilities for downstream UDS implementation.

### 15.6 Persistent Crash and Event Logging
For postmortem evidence and field debugging:
* **Retained Memory Logs**: last-reset reason, fault ring buffer.
* **Tracing History**: panic crash summaries, bus-off / error storm history, watchdog reset breadcrumbs, boot timeline tracing.

### 15.7 Deterministic Sensor Acquisition Framework
Instead of a pile of unrelated drivers, sensor inputs are unified:
* **Supported sensors**: wheel speed, IMU, steering angle, brake pressure, throttle position, battery/thermal sensors, motor resolver/QEI.
* **Features**: triggered sampling, bounded acquisition latency, timestamped samples, calibration tables, plausibility checks, stale-data detection, signal quality flags.

### 15.8 Actuator Driver Framework
Output handling must strictly adhere to safety constraints:
* **PWM Framework**: output capability with safety bounds.
* **GPIO Output Safety Profiles**: safe output ownership rules and GPIO output capability.
* **Output Policies**: actuator arming/disarming states, output clamping, slew limits, deadman timeouts, neutral/failsafe position enforcement, cross-checking for redundant outputs.

### 15.9 Health Monitoring and Redundancy Hooks
Crucial for safety-aware mobility (EVs, drones):
* **Redundancy**: redundant sensor vote hooks, mismatch detection.
* **Orchestration**: plausibility monitors, degraded mode policies, channel switchover hooks.

### 15.10 Thermal and Resource Management
Coordinated derating for vehicle electronics:
* **Thermal Management**: thermal sensor framework, thermal trip policies.
* **Derating Integration**: CPU/peripheral derating hooks, actuator derating interface.
* **Safety Integration**: overload counters, safe degradation path under heat or undervoltage.

### 15.11 Central Subsystem Managers
These ECU-level managers elevate the OS to a true automotive platform:
* **Communication Manager**: owns CAN, LIN, routing rules, diagnostic addressing, wake-triggered network behavior.
* **Vehicle State Manager**: tracks ignition, drive-ready, limp-home, fault-inhibit, charging, maintenance states.
* **Signal Manager**: normalizes vehicle signals, provides freshness/quality flags, mediates calibration/scaling, centralizes plausibility logic.
* **Safety Manager**: owns fault policy, safe-state transitions, watchdog integration, health verdicts.
* **Parameter/Config Manager**: owns calibration values, runtime parameters, configuration versioning, safe commits, fallback to defaults.

## 16. Component-Level Automotive Requirements

Specific driver families must be upgraded to support strict automotive capability requirements.

### 16.1 Advanced CAN Profile
Beyond basic CAN/FD support, an automotive profile requires:
* **Hardware Filter Compiler**: merge subscription filters into controller acceptance filters, with clean software fallback.
* **Tx Mailbox Scheduling Policy**: Class A (safety critical), Class B (real-time control), Class C (diagnostics/best effort).
* **Bus Load Monitoring**: rolling utilization, arbitration loss stats, dropped frame stats.
* **Error Confinement Diagnostics**: TEC/REC tracking, error passive/bus-off history.
* **Gateway Mode**: controlled forwarding between multiple CAN buses (crucial for body/domain controllers).
* **Diagnostic Transport Readiness**: reserved hooks for ISO-TP, UDS, OBD-II.

### 16.2 LIN Support
LIN is heavily used for low-cost actuators and sensors bridging to CAN. Minimum support requires:
* LIN master driver and schedule tables.
* Checksum handling and sleep/wake functionality.
* Bridge hooks into the central communication service.

### 16.3 Motor Control Drivers
* Synchronized PWM update points, ADC trigger sync.
* Fault input trip support, dead-time configuration.
* Complementary PWM outputs, resolver/QEI hooks.
* Immediate hardware overcurrent shutdown path.

### 16.4 ADC Drivers
* Sequencer support and trigger-based sampling.
* DMA support, calibration/offset storage.
* Channel grouping by timing class.

### 16.5 SPI/I2C Sensor Drivers
* Deterministic transfer windows, bus recovery logic.
* Device timeout handling, stale-data checks, hardware CRC handling.

### 16.6 EEPROM / NVRAM / Flash Storage
* Wear-aware parameter storage, power-loss-safe commits.
* Redundant record formats for safety-critical parameters.

## 17. Architecture Style Improvements

To safely operate in mixed-criticality environments, the core architecture enforces strict automotive idioms:

* **Capability and Ownership Model**: every critical device has clear ownership. Only one owner configures the controller; clients have restricted read/write capabilities. Explicit mode-change authority is required, and shared unsupervised MMIO access is prohibited.
* **Bounded Memory Behavior**: automotive fast paths (especially ISRs and drivers) strictly avoid heap allocation, unpredictable lock chains, unbounded queues, and broadcast wakeups.
* **Service Restartability**: noncritical services must be restartable without crashing the kernel, wedging hardware devices, or corrupting active communication state.
* **Traceability**: every safety-relevant subsystem requires a documented requirement, design component mapping, unit test case, fault injection case, and explicit degraded behavior definition.

## 18. Identified Gaps and Implementation Plan

To make the automotive architecture immediately actionable, the following gaps are identified against typical EV/automotive ECU requirements.

### 18.1 Gap Matrix

| Area | Current State | Gap | Implementation Target |
|---|---|---|---|
| Vehicle network stack | CAN/LIN/TSN/SOME-IP is listed at architecture level | **Implemented**: Userspace CAN service (`services/can`) and generic controller core (`drivers/can_core`) via capabilities | Evolve to full VNS with TSN/SOME-IP integration |
| Safety I/O path | Safety domain responsibilities are defined | Missing explicit low-latency path from fault signal to actuator-safe state | Add `Safety I/O Subsystem` with interrupt-to-action budget and emergency action table |
| Time sync | Monotonic/cross-core sync is declared | Missing production-ready API for PTP/TSN discipline and drift diagnostics | Add `Time Sync Subsystem` with clock discipline hooks and diagnostics counters |
| Power state control | Fast boot and mixed-criticality are stated | Missing runtime policy for ignition, sleep, wake, limp-home transitions | Add `Power & Mode Manager` subsystem integrated with Safety + Control domains |
| CAN controller support | CAN/CAN FD is required | **Implemented**: Generic `can_controller_ops_t` and `virt_can` backend for test/loopback | Add real hardware drivers (e.g., STM32 FDCAN, NXP FlexCAN) |
| Motor control peripheral stack | Control runtime lists motor loops | Missing standardized PWM/ADC/encoder driver contract for FOC pipelines | Add `Motor Control Driver Suite` (PWM trigger, ADC sync, QEI encoder) |
| Sensor bus integration | Sensor fusion is listed | Missing deterministic IMU + wheel-speed acquisition driver patterns | Add `Deterministic Sensor Driver Framework` with timestamped DMA ring ingestion |

### 18.2 Vehicle Network Subsystem (VNS) & CAN Architecture

The first iteration of the Vehicle Network Subsystem for CAN has been implemented following the multi-kernel capability model:

**Scope**

- Canonical frame descriptor (`can_frame_t`): FD-ready, timestamped, extensible.
- Generic Controller Interface (`can_controller_ops_t`): abstracted HAL contract for hardware drivers.
- **Kernel/HAL shim**: The kernel only handles IRQs, MMIO access, and fast-path enqueue/dequeue.
- **Userspace CAN Service**: A dedicated `can_service` handles routing, software filtering, policy enforcement, and client subscriptions via IPC. Capabilities dictate transmit/receive rights per client.

This split keeps the trusted kernel surface small, allows for restartable CAN routing logic, and isolates safety-critical policy from raw register manipulation.

### 18.3 Execution Backlog Priorities

Instead of purely driver-focused execution, implementation priority favors control-plane and safety-plane robustness:

**Priority 1**
- Safety supervisor
- Watchdog framework
- CAN hardware filter compilation
- DTC/event logging core
- Power/mode manager baseline
- Deterministic sensor framework base classes

**Priority 2**
- Motor control driver framework
- Actuator safety output framework
- Timestamp service
- Health monitoring / fault manager
- Retained crash/event log

**Priority 3**
- LIN subsystem
- Gateway service between buses
- Calibration/config persistence
- Thermal/resource derating
- Protocol layers like ISO-TP / UDS hooks
