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

## 13. Engineering Roadmap

- **Phase 1**: deterministic scheduler core + fast boot + HAL cleanup
- **Phase 2**: Linux companion interoperability (RPMsg/shared memory)
- **Phase 3**: protocol stack depth (CAN/LIN/TSN/SOME-IP)
- **Phase 4**: secure boot + firmware update + isolation hardening
- **Phase 5**: distributed ECU orchestration and validation toolchain

## 14. Target Applications

- electric mobility platforms
- autonomous robots and drones
- industrial automation
- smart mobility and gateway systems
