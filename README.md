# Bharat-OS

<p align="center">
  <img src="delivery/assets/branding/logo-icon.png" alt="Bharat-OS logo" width="140" />
</p>

<p align="center">
  <img src="delivery/assets/branding/banner-dark.png" alt="Bharat-OS banner" />
</p>

<p align="center"><em>Official Bharat-OS logo and banner assets</em></p>

---

Bharat-OS is a capability-oriented microkernel project with a multikernel direction. The repository currently delivers a **bootable and testable kernel baseline** plus architecture documentation for deferred and experimental tracks.

For our detailed vision and current subsystem maturity regarding the transition to a Barrelfish-like multikernel (supporting RT, GP, and Hybrid profiles), please see the [Multikernel Vision and Maturity Document](docs/architecture/core/MULTIKERNEL_VISION.md).

## Project status at a glance

| Area                                              | Current status                          | Notes                                                                                                                                                   |
| ------------------------------------------------- | --------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Kernel architecture                               | Baseline implemented                    | Capability objects, syscall/uAPI surfaces, core scheduler hooks, and architecture-specific HAL paths are present for x86_64, riscv64, and arm64 builds. |
| User-space service layer                          | Mixed (stubs + partial implementations) | Many services currently compile as lifecycle/event-loop stubs, while networking (`netmgr`, `netstack`) and crypto include concrete module logic.        |
| Filesystem & Storage                              | Partial scaffold                        | VFS kernel headers present; `services/file_system` is a compiled stub. Persistent storage (FAT/littlefs) and OTA recovery support are roadmap items.    |
| Networking split (`net` -> `netmgr` + `netstack`) | In progress                             | `netmgr` has control-plane tables + IPC op dispatch; `netstack` includes IPv4, ARP, ICMP, UDP, socket table, and loopback/ethernet paths.               |
| Build & test infrastructure                       | Active baseline                         | CMake presets/toolchains and host test integration are wired; architecture runtime maturity differs by target.                                          |
| Distributed/multikernel scale-out                 | Early baseline                          | Messaging-first direction is reflected in subsystem and service decomposition, but many production control-plane behaviors remain roadmap items.        |
| Documentation coverage                            | Expanded in this update                 | See `docs/dev/current-code-status.md` for a code-backed implementation matrix across services/subsystems.                                               |

For architecture-level details and deferred boundaries, see `docs/architecture/` and ADRs in `docs/adr/`. For the step-by-step closure plan, see `docs/architecture/memory-gap-closure-plan.md`. For our profile-driven, capability-safe communications and networking architecture, see [`docs/architecture/network-architecture.md`](docs/architecture/network-architecture.md).
For the ARM32/RV32 EDGE-tier expansion strategy and capability matrix, see [`docs/architecture/arm32-rv32-edge-tier-plan.md`](docs/architecture/arm32-rv32-edge-tier-plan.md).
For cross-tool code-agent guidance, guardrails, and skill templates, see [`docs/ai-agents/README.md`](docs/ai-agents/README.md).

For a code-backed snapshot of what is implemented vs. stubbed right now, see [`docs/dev/current-code-status.md`](docs/dev/current-code-status.md).

## Documentation maturity model and governance

To reduce roadmap drift and keep implementation claims accurate, Bharat-OS uses a canonical four-level maturity taxonomy across roadmap/status documentation:

- **Scaffold**: buildable placeholders and TODO loops.
- **Partial**: concrete logic exists but end-to-end behavior is incomplete.
- **Baseline**: core path works for integration/developer workflows.
- **Production**: hardened and validated for reliability, security, and operations.

Primary references:

- [`docs/dev/current-code-status.md`](docs/dev/current-code-status.md) — code-backed status truth source.
- [`ROADMAP.md`](ROADMAP.md) — forward-looking plan with explicit maturity labels and discrepancy log.
- [`docs/architecture/cmake-governance-and-agent-rules.md`](docs/architecture/cmake-governance-and-agent-rules.md) — CMake structure, versioning policy, and contributor/agent rules.
- [`docs/architecture/components/kernel-subcomponents-architecture.md`](docs/architecture/components/kernel-subcomponents-architecture.md) — kernel subcomponent architecture, per-arch notes, done/todo, roadmap map.
- [`docs/architecture/components/subsystem-subcomponents-architecture.md`](docs/architecture/components/subsystem-subcomponents-architecture.md) — subsystem architecture and closure map.
- [`docs/architecture/components/services-subcomponents-architecture.md`](docs/architecture/components/services-subcomponents-architecture.md) — service-domain architecture/status map.
- [`docs/architecture/components/drivers-subcomponents-architecture.md`](docs/architecture/components/drivers-subcomponents-architecture.md) — driver architecture and arch constraints map.

We intentionally keep architecture documents forward-looking while maintaining conservative implementation status in `docs/dev/current-code-status.md`.

## Device Profiles & Use-cases

Bharat-OS is intentionally profile-driven: the same small capability-oriented kernel can be composed with different services, drivers, stacks, and personalities depending on the device class. This avoids building a separate OS for every product category while still allowing deterministic, safety-aware, and resource-bounded deployments.

> Current status: these profiles describe Bharat-OS architecture direction and implementation mapping. They are not yet production-certification claims. Safety, automotive, aerospace, and industrial deployments require additional validation, fault-containment, timing analysis, certification artifacts, and hardware-specific qualification.

- **Mobile / Wearables (EDGE profile):** capability isolation, bounded footprint, and power-aware scheduling hooks are available now; production-grade power control policy is roadmap.
- **Robotics / Drones (EDGE + RTOS-leaning):** deterministic IPC pathways and architecture portability are present; stronger real-time admission and fault-containment depth are roadmap.
- **Network appliances / Edge gateways:** capability-mediated driver boundaries and multikernel messaging baseline are present; mature data-plane acceleration is roadmap.
- **Data-center / clustered nodes:** NUMA/multicore scaffolding and URPC primitives are present; full distributed scheduling and high-scale service orchestration are roadmap.

For automotive, industrial, aerospace, and cockpit profile mapping, see [Device Profiles and Use Cases](docs/architecture/device-profiles-and-use-cases.md).
For safety-profile maturity and certification-readiness gaps, see [Safety-Oriented Profile Roadmap](docs/architecture/safety-oriented-profile-roadmap.md).

### Safety-Oriented Profile Composition

```mermaid
graph TD
    subgraph Apps["Apps / Domain Workloads"]
        ADAS["ADAS / Perception"]
        Cockpit["Digital Cockpit"]
        Control["Industrial / Aerospace Control"]
        Gateway["Gateway ECU"]
    end

    subgraph Services["User-space Services"]
        SafetyMgr["Safety Manager"]
        Telemetry["Telemetry & Diagnostics"]
        DeviceMgr["Device Manager"]
        NetMgr["Network / Gateway Manager"]
        VehicleStack["Vehicle / Sensor / Actuator Stack"]
        DisplayMgr["Display / Cockpit UI Manager"]
        AIGov["AI / Accelerator Governor"]
    end

    subgraph Kernel["Bharat-OS Microkernel"]
        Cap["Capabilities"]
        IPC["IPC / URPC"]
        Sched["Scheduler / Deadlines"]
        MM["VMM / PMM"]
        Fault["Fault Domains"]
    end

    subgraph HW["Hardware / HAL"]
        CPU["CPU Cores"]
        Sensors["Sensors / Buses"]
        Accel["GPU / NPU / DSP"]
        Display["Display"]
        Network["Ethernet / CAN / Radio"]
    end

    ADAS --> VehicleStack
    Cockpit --> DisplayMgr
    Control --> SafetyMgr
    Gateway --> NetMgr

    VehicleStack --> IPC
    DisplayMgr --> IPC
    NetMgr --> IPC
    SafetyMgr --> Fault
    Telemetry --> IPC
    AIGov --> Sched

    IPC --> Cap
    Sched --> CPU
    MM --> CPU
    Cap --> Sensors
    Cap --> Accel
    Cap --> Display
    Cap --> Network
```

### ADAS / Autonomous Driving

Bharat-OS is a good fit for future ADAS and autonomous-driving control platforms because the architecture separates safety-critical control paths from policy-heavy user-space services.

Potential value:

- **Mixed-criticality partitioning:** perception, planning, control, telemetry, diagnostics, and UI can run in isolated domains with explicit capabilities.
- **Deterministic IPC/URPC:** low-latency message paths can connect camera/radar/lidar preprocessing, fusion, planning, and actuator-control services without making the kernel policy-heavy.
- **Restartable user-space drivers:** sensor or accelerator drivers can fail and restart without forcing whole-system failure.
- **AI/accelerator-aware scheduling roadmap:** AI inference workloads can be placed by a user-space governor while the kernel keeps bounded deterministic fallback behavior.
- **Fault-domain design path:** perception failure, sensor timeout, control-path deadline miss, and driver crash can be classified into fault domains and routed to safe-state policy.

Example mapping:

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

Potential value:

- **Capability-mediated networking:** each network interface, bus, or diagnostic endpoint can expose only the rights required by that service.
- **Gateway service isolation:** routing, filtering, diagnostics, OTA, and telemetry can be separate services rather than one privileged monolith.
- **Restartable communication stacks:** CAN, Ethernet, SOME/IP-like services, diagnostics, and update channels can restart independently.
- **Policy outside kernel:** routing, firewall, diagnostic access, and update policy stay in services, keeping the kernel smaller.

Example mapping:

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

Potential value:

- **RT-leaning profile:** deterministic memory allocation, deadline metadata, and bounded IPC paths can support PLC-like and motion-control workloads.
- **Sensor/actuator isolation:** actuator control can be capability-gated and separated from HMI, logging, and remote-management services.
- **Fault containment:** a failed HMI, logging service, or network service should not directly corrupt control-loop execution.
- **Small deployment footprint:** profile-based composition can disable unnecessary media, desktop, or cloud features.

Example mapping:

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

Potential value:

- **Minimal trusted kernel base:** only scheduling, memory, traps, capabilities, IPC/uRPC, and fault handling should remain in kernel.
- **Strict domain separation:** navigation, telemetry, payload, communications, and diagnostics can run as isolated services.
- **Fault-domain model:** service failure can be isolated and escalated to safe-mode policy instead of uncontrolled system-wide failure.
- **Cross-architecture portability:** x86_64, ARM64, and RISC-V targets allow experimentation across development boards, simulators, and future custom silicon.

Example mapping:

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

Potential value:

- **Domain split:** infotainment, cluster, navigation, voice assistant, media, and vehicle-status display can be separated.
- **Display tiers:** headless, text console, framebuffer, lightweight embedded UI, and full compositor can be enabled by profile.
- **Multi-personality direction:** Linux/Android personality work can support app/runtime compatibility without moving those compatibility layers into the kernel.
- **Safety-aware UI isolation:** cluster-critical display paths can be separated from entertainment or third-party app domains.

Example mapping:

| Cockpit function | Bharat-OS mapping |
| --- | --- |
| Instrument cluster | Lightweight UI/framebuffer service |
| Infotainment | Media stack + Android/Linux personality path |
| Navigation | App/service domain |
| Vehicle status | Read-only vehicle data capability |
| Voice/AI assistant | User-space AI/accelerator service |
| Secure warning overlay | Trusted UI/display broker roadmap |

### Why Bharat-OS is interesting for these domains

Bharat-OS is not trying to be a monolithic general-purpose OS. Its value is in a smaller capability-oriented kernel plus profile-selected services:

- **Capability-based security:** access to memory, devices, IPC endpoints, and services is explicit and revocable.
- **Microkernel layering:** policy-heavy behavior stays outside ring-0.
- **Multikernel direction:** cross-core coordination uses explicit messaging rather than hidden global sharing.
- **Profile-aware composition:** builds can select only the services needed for automotive, industrial, aerospace, cockpit, edge, or cloud profiles.
- **Restartable services and drivers:** user-space services can be supervised, restarted, or isolated.
- **Safety roadmap:** watchdogs, fault domains, deadline primitives, telemetry, and safe-state transitions can become common reusable foundations.

### Near-term maturity gaps for safety-oriented profiles

Before claiming production readiness for ADAS, gateway ECUs, industrial control, aerospace, or cockpit systems, Bharat-OS must close these gaps:

- bounded scheduler and cross-core migration behavior
- bounded TLB shootdown and memory invalidation protocol
- strict capability enforcement at every IPC/service boundary
- real service supervisor with restart/backoff/watchdog policy
- fault-domain and safe-state transition framework
- deadline/timer primitives with tests
- device-class registry for sensors, actuators, display, network, storage, and accelerator devices
- telemetry and diagnostic event service
- clear separation between kernel mechanisms, services, stacks, and domain personalities

### High-Level Architecture

```mermaid
%%{init: {'theme': 'base', 'themeVariables': { 'fontFamily': 'arial', 'fontSize': '14px', 'primaryColor': '#0d4f40', 'edgeLabelBackground':'#ffffff'}}}%%
graph TD
    classDef default fill:#0d4f40,stroke:#0d4f40,color:#fff,rx:10px,ry:10px;

    classDef greenNode fill:#0d4f40,stroke:#1d8c6b,stroke-width:2px,color:#fff,rx:5px,ry:5px;
    classDef orangeNode fill:#874900,stroke:#f8a619,stroke-width:2px,color:#fff,rx:5px,ry:5px;
    classDef purpleNode fill:#3f3281,stroke:#6655a6,stroke-width:2px,color:#fff,rx:5px,ry:5px;
    classDef grayNode fill:#4b4d48,stroke:#868884,stroke-width:2px,color:#fff,rx:5px,ry:5px;

    classDef invisible fill:none,stroke:none,color:#999;

    subgraph US [User-space domains<br/>Policy, drivers, subsystems — isolated by capabilities]
        direction LR
        S1["<b>Subsystems</b><br/><span style='color:#75bfa9; font-size:12px'>Console, FB, Input</span>"]:::greenNode
        S2["<b>Drivers</b><br/><span style='color:#75bfa9; font-size:12px'>Net, Storage, Display</span>"]:::greenNode
        S3["<b>AI Governor</b><br/><span style='color:#f8a619; font-size:12px'>Sched hints, telemetry</span>"]:::orangeNode
        S4["<b>Memory Policy</b><br/><span style='color:#75bfa9; font-size:12px'>RT static / Cloud NUMA</span>"]:::greenNode
    end

    subgraph MK [Microkernel ring-0<br/>Trusted compute base — capability tables, IPC, VMM, scheduler]
        direction LR
        M1["<b>Capability</b><br/><span style='color:#a89cd4; font-size:12px'>invoke/grant/revoke</span>"]:::purpleNode
        M2["<b>IPC / URPC</b><br/><span style='color:#a89cd4; font-size:12px'>Sync + lockless ring</span>"]:::purpleNode
        M3["<b>VMM / PMM</b><br/><span style='color:#a89cd4; font-size:12px'>Buddy + slab alloc</span>"]:::purpleNode
        M4["<b>Scheduler</b><br/><span style='color:#a89cd4; font-size:12px'>Tick-driven + AI hooks</span>"]:::purpleNode
    end

    subgraph HL [Hardware abstraction layer<br/>Portability contracts across architectures]
        direction LR
        H1["<b>x86_64</b>"]:::grayNode
        H2["<b>arm64 / ARMv8</b>"]:::grayNode
        H3["<b>RISC-V / Shakti</b>"]:::grayNode
    end

    US -->|syscall / IPC| MK
    MK -->|HAL call| HL

    style US fill:#0d4f40,stroke:#0d4f40,color:#fff,rx:10px,ry:10px
    style MK fill:#3f3281,stroke:#3f3281,color:#fff,rx:10px,ry:10px
    style HL fill:#4b4d48,stroke:#4b4d48,color:#fff,rx:10px,ry:10px

    subgraph Legend [ ]
        direction LR
        L1["<div style='background-color:#1d8c6b; width:15px; height:15px; border-radius:3px; display:inline-block; vertical-align:middle;'></div> <span style='color:#999;'>Baseline implemented</span>"]:::invisible
        L2["<div style='background-color:#f8a619; width:15px; height:15px; border-radius:3px; display:inline-block; vertical-align:middle;'></div> <span style='color:#999;'>Partial / scaffolding</span>"]:::invisible
        L3["<div style='background-color:#999999; width:15px; height:15px; border-radius:3px; display:inline-block; vertical-align:middle;'></div> <span style='color:#999;'>Deferred / roadmap</span>"]:::invisible
    end
    style Legend fill:none,stroke:none

    HL ~~~ Legend

    linkStyle 0 stroke:#999,stroke-width:2px,color:#999
    linkStyle 1 stroke:#999,stroke-width:2px,color:#999
```

### Key Technical Pillars

- **Tiered Functionality:** The OS scales its footprint by activating specific Tiers. Small devices run **Tier A** (minimal core), while desktops and servers enable **Tiers B and C** for full POSIX and GUI support.
- **Multi-Architecture HAL:** Native support for `x86_64`, `ARMv8`, and notably **Shakti RISC-V**, ensuring performance on local semiconductor innovations.
- **Distributed IPC:** A capability-based IPC model that treats local and remote system calls through a unified messaging interface.

### Current v1 Architecture Highlights

| Feature                          | Summary                                                                                                                                                  |
| :------------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Verification-first microkernel   | Ring-0 keeps boot flow, memory mapping, capability tables, IPC, and scheduler scaffolding; policy/services stay in isolated user-space domains.          |
| Capability-based security model  | No global ACL/root model in kernel; object access is capability-mediated (`invoke`, `grant`, `revoke`, `retype`) with zero-trust isolation.              |
| Flexible memory model            | Kernel maps/unmaps physical pages, while memory policy remains in user space (Bharat-RT static/no-paging; Bharat-Cloud demand paging + NUMA-aware path). |
| Synchronous and asynchronous IPC | Fast register-based endpoint IPC for low latency plus lockless ring-buffer URPC for cross-core multikernel messaging.                                    |
| User-space driver model          | Drivers are unprivileged; capabilities gate MMIO/IRQ access and IOMMU policy hardens DMA boundaries, enabling restartable driver domains.                |
| Modular scheduler with AI hooks  | Tick-driven scheduler collects telemetry and applies AI hints via ADR-008 plugin boundaries, with deterministic fallback when PMCs are unavailable.      |

#### Multi-Personality Subsystem Architecture

The Bharat-OS multi-personality strategy does not bake monolithic compatibility subsystems into the core kernel. Instead, it maintains a small, verifiable, distributed kernel that exposes personality-neutral primitives (tasks, memory objects, capabilities). Layered compatibility subsystems translate these core primitives into personality-specific abstractions (Linux POSIX, Android, Windows NT).

```mermaid
%%{init: {'theme': 'base', 'themeVariables': { 'primaryColor': '#cceeff', 'edgeLabelBackground':'#ffffff'}}}%%
graph TD
    subgraph Personalities [User-Space Personalities & Apps]
        subgraph Android [Android Personality]
            ART[ART/Dalvik Runtime]
            Binder[Binder Compat]
            Ashmem[Ashmem Shim]
        end

        subgraph Linux [Linux Personality]
            POSIX[POSIX / VFS]
            Signals[Signals / Futex]
            ELF[ELF Loader]
        end

        subgraph Native [Bharat-OS Native]
            NativeApps[Native Apps]
            NativeServices[Native Services]
        end
    end

    subgraph Microkernel [Bharat-OS Core Microkernel Ring-0]
        CoreSched[Personality-Aware Scheduler]
        CoreMM[Memory Mapping & Slab]
        CoreIPC[URPC & Sync IPC]
        CoreCap[Capability System]
    end

    ART --> Binder
    ART --> Ashmem
    Binder --> CoreIPC
    Ashmem --> CoreMM

    POSIX --> CoreMM
    Signals --> CoreSched
    ELF --> CoreMM

    NativeApps --> NativeServices
    NativeServices --> CoreIPC

    Android -.->|Translated Syscalls| Microkernel
    Linux -.->|Translated Syscalls| Microkernel
    Native -.->|Native Invokes| Microkernel
```

#### Capability Model Architecture

Bharat-OS enforces security through a mathematically verifiable Capability System. There are no global Access Control Lists (ACLs), user IDs, or root privileges inside the kernel. A capability is an unforgeable, kernel-managed token that pairs an object reference with a set of permitted operations.

```mermaid
%%{init: {'theme': 'base', 'themeVariables': { 'primaryColor': '#eebbff', 'edgeLabelBackground':'#ffffff'}}}%%
graph TD
    subgraph Thread [Thread Domain]
        CSpace[Capability Space]
        Cap1[Cap: Read/Write]
        Cap2[Cap: Execute]
        Cap3[Cap: Grant]

        CSpace --> Cap1
        CSpace --> Cap2
        CSpace --> Cap3
    end

    subgraph Operations [Capability Operations]
        Invoke[Invoke: Perform action]
        Grant[Grant: Transfer over IPC]
        Revoke[Revoke: Invalidate cap]
        Retype[Retype: Convert memory]
    end

    subgraph KernelObjects [Kernel Objects]
        Mem[Memory Frames]
        EP[IPC Endpoints]
        T[Threads]
        U[Untyped Memory]
    end

    Cap1 --> Invoke
    Cap2 --> Invoke
    Cap3 --> Grant

    Invoke --> Mem
    Invoke --> EP
    Grant --> T
    Retype --> U
```

#### Memory Management Architecture

```mermaid
%%{init: {'theme': 'base', 'themeVariables': { 'primaryColor': '#ddffcc', 'edgeLabelBackground':'#ffffff'}}}%%
graph TD
    subgraph UserSpace [User-Space Memory Policy]
        BharatRT[Bharat-RT Static / No-Paging]
        BharatCloud[Bharat-Cloud Demand Paging & NUMA]
    end

    subgraph Microkernel [Bharat-OS Microkernel Ring-0]
        VMM[Virtual Memory Management - Mapping Registry]
        PMM[Physical Memory Management - Buddy Allocator]
        Slab[Slab Allocator - kvmalloc/kvfree]
    end

    subgraph HAL [Hardware Abstraction Layer]
        PageTable[Architecture Page Tables - x86_64 / arm64 / riscv64]
    end

    BharatRT -->|Mapping Requests| VMM
    BharatCloud -->|Page Faults & Mapping Requests| VMM

    VMM -->|Requires Physical Memory| PMM
    VMM -->|Manipulates| PageTable

    Slab -->|Allocates Metadata & Pages| VMM
    Slab -->|Backed by| PMM
```

#### IPC & Messaging Architecture

We utilize two distinct IPC models to serve both deterministic bounds (Bharat-RT) and massive scalability (Bharat-Cloud). **Synchronous Endpoint IPC** is fast, blocking, and unbuffered for strict procedural calls. **Lockless URPC** (User-level Remote Procedure Call) is designed for cross-core, multikernel messaging, scaling across high-core-count processors without shared-kernel locks.

```mermaid
%%{init: {'theme': 'base', 'themeVariables': { 'primaryColor': '#ffeecc', 'edgeLabelBackground':'#ffffff'}}}%%
graph TD
    subgraph UserSpace [User-Space Domains]
        Sender[Sender Domain]
        Receiver[Receiver Domain]
    end

    subgraph Capability [Capability System]
        CapTable[Capability Tables]
        Checks[Delegated Rights Checks]
    end

    subgraph Microkernel [Bharat-OS Microkernel]
        subgraph SyncIPC [Synchronous Endpoint IPC]
            RegisterPass[Register-based Message Passing]
        end

        subgraph AsyncIPC [Lockless URPC Multikernel Messaging]
            RingBuffer[Lockless Shared Ring Buffer]
            MultiCore[Cross-Core Interconnect Transport]
        end
    end

    Sender -->|Invoke Capability| CapTable
    Receiver -->|Invoke Capability| CapTable

    CapTable --> Checks
    Checks -->|Verified Access| SyncIPC
    Checks -->|Verified Access| AsyncIPC

    RegisterPass -->|Low-latency Register Delivery| Receiver
    RingBuffer --> MultiCore
    MultiCore -->|Async Distributed / NUMA Delivery| Receiver
```

### Hardware Profiles & Boot Flow

Bharat-OS is intentionally profile-driven instead of forcing one heavyweight image on every board. Boot behavior and subsystem initialization are determined dynamically by the detected hardware profile.

```mermaid
%%{init: {'theme': 'base', 'themeVariables': { 'primaryColor': '#ffccff', 'edgeLabelBackground':'#ffffff'}}}%%
graph LR
    subgraph CoreBoot [Core Kernel Bring-up]
        HAL[HAL & Arch Init] --> Cap[Capability Table Init]
        Cap --> Sched[Scheduler Setup]
    end

    subgraph ProfileDispatch [Profile Detection]
        Sched --> Detect{Hardware Profile}
    end

    subgraph RTOS [RTOS / Safety Profile]
        Static[Static Memory Pools]
        Watchdog[Watchdog / Health Monitor]
        DetermSched[Deterministic Sched]
    end

    subgraph Edge [Edge / IoT Profile]
        OTA[Secure Boot / OTA]
        PwrMgmt[Power Management]
        FB[Framebuffer UI]
    end

    subgraph Datacenter [Datacenter / Cloud Profile]
        NUMA[NUMA-aware Memory]
        ScaleSched[Scalable Runqueues]
        Virt[VirtIO / VM Hooks]
    end

    Detect -->|RTOS| RTOS
    Detect -->|Edge| Edge
    Detect -->|Datacenter| Datacenter
```

### Subsystem Model

Bharat-OS defines explicit subsystem groups to ensure scalable and tailored functionality for every device class:

- **Console Subsystem:** Serial and text console outputs for early bring-up, logging, and headless environments.
- **Framebuffer & Embedded Graphics Subsystem:** The _primary_ graphics path for small devices. Framebuffers are treated as a first-class target, offering deterministic rendering and software-rendered UI without dragging in a heavy GPU compositor.
- **Input Subsystem:** Modular routing for keyboards, touch panels, rotary encoders, and GPIO buttons.
- **Heterogeneous Accelerator Subsystem:** DMA engines, DSPs, NPUs, and ISP abstractions for edge AI and multimedia tasks.
- **Embedded Device Services:** Kiosk shells, watchdog timers, OTA recovery, and lightweight local storage.
- **Filesystem & Storage Subsystem:** VFS abstraction mapping capability-based IO to block, blob, and persistent storage drivers (e.g., FAT, littlefs) necessary for stateful edge devices and OTA recovery.
- **Desktop Graphics Subsystem:** An advanced layer reserved for devices with capable hardware and full compositor needs.

### Display & GUI Strategy

Our display architecture explicitly rejects "desktop compositor or nothing". We define output subsystems in layers:

1. **Headless:** Remote management and serial outputs (Tier 0).
2. **Text console:** VGA/serial output for basic bring-up (Tier 1).
3. **Framebuffer graphics:** Simple 2D display operations and robust device driver abstractions (Tier 2).
4. **Embedded lightweight UI:** Direct-rendered widgets or lightweight toolkits tailored for kiosks and industrial panels (Tier 3).
5. **Full compositor / desktop GUI:** Accelerated environments for workstations and advanced infotainment displays (Tier 4).

---

## 🧭 Roadmap (Condensed)

The official roadmap for the multikernel migration and continuous evolution of Bharat-OS is detailed in [`ROADMAP.md`](ROADMAP.md). Highlights include:

- **Phase 1 (Multikernel Foundation & UI):** Per-CPU capability and runqueue isolation, message-based TLB shootdowns, URPC topology maps (SKB), **framebuffer core**, and **text output.**
- **Phase 2 (Device Specialization & Edge UI):** Hardware validation, AI predictive heuristics from PMC counters, **touch/key input**, and small-device UIs.
- **Phase 3 (Cloud / Datacenter):** NUMA scale-up, Zero-copy networking, DMA/accelerator orchestrations.
- **Phase 4 (Advanced UX & Vertification):** Full compositor environments and Isabelle/HOL formal verification layers.

### AI Features & Roadmap

- Current kernel scheduler tracks thread telemetry (`cycles`, `instructions`, `CPI`) and accepts AI suggestions through a bounded, testable path.
- Architecture-specific PMCs can be sampled when available; deterministic approximations are used otherwise.
- ADR-008 defines the plugin boundary so scheduler core remains portable while profile/architecture overrides evolve.
- Near-term extensions include user-space AI governors, profile-aware scheduling heuristics, and accelerator-aware placement for edge/cloud workloads.

---

## 🧠 AI-Driven Resource Management

Detailed mapping is documented in [`docs/architecture/device-profiles-and-use-cases.md`](docs/architecture/device-profiles-and-use-cases.md).

## AI Features & Roadmap

Bharat-OS keeps AI policy outside ring-0 while exposing bounded kernel mechanisms:

### Implemented baseline

- Kernel-side telemetry collection hooks and bounded AI suggestion queueing.
- Scheduler action handling for migrate/priority/throttle suggestion types.
- Capability-guarded governor control-plane endpoint.
- Architecture/profile-neutral telemetry plugin contract (with fallback behavior when PMCs are unavailable).

#### Scheduler & AI Hooks Architecture

```mermaid
%%{init: {'theme': 'base', 'themeVariables': { 'primaryColor': '#ccffdd', 'edgeLabelBackground':'#ffffff'}}}%%
graph TD
    subgraph UserSpace [User-Space Policy]
        AIGov[AI Governor]
        SystemProfile[Profile-Aware Heuristics - Tier A/B/C]
        TelemetryCollection[Telemetry Ingestion]
    end

    subgraph Microkernel [Bharat-OS Microkernel Ring-0]
        Sched[Tick-driven Scheduler]
        Telemetry[Kernel Telemetry Collection - Cycles, Instructions, CPI]
        AIHooks[Bounded AI Suggestion Queue - Migrate, Priority, Throttle]
        Fallback[Deterministic Fallback - Round Robin]
    end

    subgraph Hardware [Hardware Abstraction Layer]
        PMCs[Hardware PMCs - Architecture Specific]
        Timer[Generic Timer Tick]
    end

    Timer -->|Tick Event| Sched
    PMCs -->|Sample Counters| Telemetry

    Sched --> Telemetry
    Telemetry -->|Expose Metrics| TelemetryCollection

    SystemProfile --> AIGov
    TelemetryCollection --> AIGov

    AIGov -->|Submit AI Suggestions| AIHooks
    AIHooks -->|Validate Bounds| Sched

    Sched -->|If PMCs / AI unavailable| Fallback
    Sched -->|Context Switch| Hardware
```

### Roadmap

- Better telemetry quality (hardware PMC integrations per architecture).
- Per-core runqueues + richer migration policy under SMP load.
- Safety/verification hardening for AI-driven scheduling decisions.
- Clearer user-space governor lifecycle, observability, and audit reporting.

See [`docs/architecture/ai-scheduler-status-and-roadmap.md`](docs/architecture/ai-scheduler-status-and-roadmap.md) and [`docs/adr/ADR-008-ai-scheduler-plugin-contract.md`](docs/adr/ADR-008-ai-scheduler-plugin-contract.md).

#### Networking Architecture

See:
[`docs/architecture/network-architecture.md`](docs/architecture/network-architecture.md)

## Core architecture themes

- **Capability-based security:** object rights, delegation constraints, and explicit authority checks.
- **Microkernel layering:** small kernel core with user-space policy and service growth path.
- **Multikernel direction:** explicit messaging-oriented coordination across cores and eventually nodes.
- **Profile-aware composition:** RTOS/EDGE/DESKTOP profile tuning with bounded kernel mechanisms.

## Build quick start

For complete setup on **Windows, WSL/Linux, and macOS** (including QEMU/OpenOCD/GDB install and full preset command cookbook), read [`BUILD.md`](BUILD.md).

### Daily commands

```powershell
# PowerShell
.\build.ps1 all --target x86_64_desktop_headless
.\build.ps1 all --target arm64_desktop_headless
.\build.ps1 all --target riscv64_desktop_headless
```

```bash
# WSL/Linux/macOS
./build.sh all --target x86_64_desktop_headless
./build.sh all --target arm64_desktop_headless
./build.sh all --target riscv64_desktop_headless
./build.sh run --target-yaml delivery/targets/qemu/arm32_mmu_lite_headless.yaml
```


### Linux + Android headless personality targets
To build and boot the Linux or Android headless targets for smoke testing, use:
```bash
./build.sh all --target x86_64_desktop_headless_linux
./build.sh all --target arm64_desktop_headless_linux
./build.sh all --target riscv64_desktop_headless_linux

./build.sh all --target x86_64_desktop_headless_android
./build.sh all --target arm64_desktop_headless_android
./build.sh all --target riscv64_desktop_headless_android
```

To run the automated E2E smoke tests for all 6 personality targets:
```bash
python3 tests/e2e/test_personalities_smoke.py
```


```bash
./build.sh all --target x86_64_desktop_headless_linux
./build.sh all --target arm64_desktop_headless_linux
./build.sh all --target riscv64_desktop_headless_linux

./build.sh all --target x86_64_desktop_headless_android
./build.sh all --target arm64_desktop_headless_android
./build.sh all --target riscv64_desktop_headless_android
```

### Stage-oriented usage

```bash
./build.sh build   --target x86_64_desktop_headless
./build.sh package --target x86_64_desktop_headless
./build.sh run     --target x86_64_desktop_headless
./build.sh flash   --target <board_target> --dry-run
```

PowerShell equivalents:

```powershell
.\build.ps1 build   --target x86_64_desktop_headless
.\build.ps1 package --target x86_64_desktop_headless
.\build.ps1 run     --target x86_64_desktop_headless
.\build.ps1 flash   --target <board_target> --dry-run
```

Legacy positional compatibility example:

```powershell
.\build.ps1 x86_64_desktop_headless --run
```

Equivalent modern command:

```powershell
.\build.ps1 all --target x86_64_desktop_headless
```

## Repository layout

- `kernel/` — microkernel core (MM, IPC, scheduler, HAL, capability system).
- `subsys/` — subsystem services (including AI governor bridge layer).
- `lib/` — shared user-space facing library surfaces.
- `quality/tests/` — canonical host/unit/e2e tests for kernel/runtime components (`tests/` remains a compatibility symlink during migration).
- `docs/` — architecture docs, ADRs, and implementation reviews.

## Research references

This project aligns with established systems research and uses those works as design guidance:

- Barrelfish multikernel model (messaging-first multicore OS design).
- seL4 capability model and verification-oriented discipline.
- L4-family microkernel separation and minimal trusted core concepts.
- AI-assisted resource management literature (policy guidance in user space, bounded kernel enforcement).

These references are informational guidance for architecture direction, not claims of feature parity.
Bharat-OS draws inspiration from and builds upon research in AI-driven systems and microkernel architectures.

### Research Inspirations

- **Barrelfish multikernel model:** treats a machine as a distributed system of cores coordinated by explicit message passing; this directly informs Bharat-OS URPC and cross-core service decomposition. ([PDF](https://sigops.org/s/conferences/sosp/2009/papers/baumann-sosp09.pdf))
- **seL4 capability model and verification-first design:** capability invocation as the primary authority path and a small trusted kernel base inform Bharat-OS object-capability isolation goals. ([PDF](https://sigops.org/s/conferences/sosp/2009/papers/klein-sosp09.pdf), [TOSEM PDF](https://trustworthy.systems/publications/nicta_full_text/7371.pdf))
- **L4 Family Microkernels:** Surveys L4 evolution, emphasizing modularity; Bharat-OS builds on L4's IPC and driver isolation principles. ([PDF](https://trustworthy.systems/publications/nicta_full_text/8988.pdf))
- **AI scheduling research:** workload-aware scheduling literature (including RL-driven resource managers) informs the long-term AI-governor and scheduler policy roadmap. ([arXiv](https://arxiv.org/abs/2403.01185), [IJMET PDF](https://iaeme.com/MasterAdmin/Journal_uploads/IJMET/VOLUME_11_ISSUE_12/IJMET_11_12_012.pdf))

For a complete bibliography and BibTeX entries, see [`docs/research_doc/papers.md`](docs/research_doc/papers.md) and [`docs/research_doc/references.bib`](docs/research_doc/references.bib).

### Phase 4 Verification Roadmap

As part of Phase 4, we plan to integrate seL4 tools for verification. Our initial focus will be on Isabelle/HOL proofs for our core IPC primitives. We are actively seeking and welcome help from other developers on this roadmap. If you have experience in formal verification or theorem proving, please join us!

## Build script hierarchy

- Root `build.sh` and `build.ps1` are the supported user-facing entrypoints.
- `tools/build.py` is the authoritative build/run implementation.
- Any shell or PowerShell scripts under `tools/` (like `tools/build.sh` and `tools/build.ps1`) are compatibility wrappers only.
- Future CLI, build, or run behavior changes must be made in `tools/build.py` only. Do not add new logic to the compatibility shims.
