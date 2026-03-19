# Bharat-OS Communications and Network Architecture

This document defines the capability-safe, profile-driven communications architecture for Bharat-OS. It outlines the strategic move from a basic network stack placeholder to a first-class communications framework designed for multiple device classes (embedded, edge gateways, automotive, robotics, industrial RT, and cloud nodes).

## Why Networking Must Be a First-Class Subsystem

For automotive, drones, industrial RT, and safety-critical devices, a "network stack" cannot mean only Ethernet, IP, and TCP. It must be a multi-bus communications architecture capable of handling real-time field buses, vehicle/avionics control buses, time-sensitive Ethernet, wireless links, gatewaying between them, and strict isolation between control-plane, telemetry, and infotainment/cloud traffic.

Because communications span across these deeply divergent and safety-critical constraints, they are elevated to a set of first-class subsystems (`subsys/network`, `subsys/automotive`, and future domains like `subsys/rtcomms`), comparable to the Memory Management and Subsystems domains.

## Why `services/net` Alone is Insufficient

The original `services/net` directory acted as a monolithic blob that attempted to hold drivers, IP stacks, and routing policy in one place. A monolithic user-space network service fails to scale and cannot support the diverse needs of modern intelligent systems:
* **Performance:** Forcing all packet processing through a single user-space domain introduces unacceptable IPC overhead and locking contention.
* **Security & Reliability:** A single crash in a generic driver or protocol parser would take down the entire communications framework.
* **Profile Flexibility:** Embedded devices cannot afford a monolithic TCP/IP stack, Edge Gateways require high-speed dataplane logic separated from configuration, and Automotive systems require deterministic buses (like CAN) entirely isolated from IP traffic.

To solve this, `services/net` is being explicitly decomposed into a multi-layered, multi-bus structure.

## Communications Families

The Bharat-OS communications framework is split into four primary families that must coexist, isolated or gatewayed, according to profile policy:

### 1. Control Buses
Used for deterministic device-to-device control.
* **Examples:** CAN / CAN-FD, LIN, FlexRay, UAVCAN / Cyphal, ARINC buses, RS-485 / Modbus.
* These are first-class transport domains, not optional drivers.

### 2. Real-Time Ethernet / Switched Deterministic Networking
Used when devices need higher bandwidth but bounded latency.
* **Examples:** TSN, automotive Ethernet, deterministic UDP/DDS, SOME/IP, gPTP / time sync.

### 3. General IP Networking
Used for standard connectivity.
* **Examples:** Cloud connectivity, diagnostics, OTA, fleet management, UI, video streams, telemetry.
* Includes normal IPv4/IPv6, TCP/UDP, DNS, DHCP, TLS.

### 4. Wireless / Mission Links
Used for diverse wireless communications.
* **Examples:** Wi-Fi, LTE/5G, BLE, telemetry radios, mesh links, GNSS-assisted comms.

## Layered Model

The Bharat-OS network architecture is split into primary layers, ensuring that policy, basic connectivity, and high-speed data flow remain isolated and configurable.

### Minimum Universal IP Core
This is the **always-on baseline** for any build requiring general networking.
* **Ownership:** `services/netstack` and `lib/net`
* **Responsibilities:**
  * NIC discovery and link state.
  * Ethernet, VLAN, ARP/NDP.
  * IPv4 and IPv6.
  * ICMP/ICMPv6, UDP, basic TCP.
  * Socket-like API contracts.
  * Basic routing and DHCP client operations.

### Profile-Specific Advanced Planes
Above the baseline, extra capabilities are activated based on the build profile (e.g., Automotive vs. Cloud).
* **Ownership:** `services/netmgr`, `services/busmgr`, `services/gatewayd`, `subsys/network`, `subsys/automotive`
* **Responsibilities:**
  * Stateful firewalls and NAT (Security Appliances).
  * Policy routing and VRF (Cloud Nodes/Gateways).
  * Multiqueue NIC scaling (Cloud Nodes).
  * CAN ↔ IP bridging, bus ↔ Ethernet translation, safety filters (Automotive/Gateway).
  * Bus capability assignment, interface lifecycle (Bus Orchestration).
  * Traffic classes and QoS policy (Drones/Automotive).

### Fast Path / Acceleration Plane (Roadmap)
A dedicated lane for line-rate packet processing that bypasses the control plane entirely.
* **Ownership:** Future `services/netfast`
* **Responsibilities:**
  * Zero-copy RX/TX rings (`lib/packet`).
  * Lock-minimized, per-core queues.
  * XDP-like early packet drop/steering.
  * Hardware accelerator / NIC DMA integration.

## Automotive, Drone, and RT Needs

Bharat-OS must support domain gateway architectures and profile-driven policies, not just protocol checklists.

### Automotive Needs
Automotive platforms need CAN/CAN-FD, LIN, Ethernet, and gatewaying between legacy buses and Ethernet. Crucially, they require strong isolation between the safety/control domain, body domain, infotainment domain, and telematics/cloud domain. This requires CAN frame APIs, bus arbitration awareness, filter configuration, timestamping, and deterministic queues.

### Drone / UAV / Robotics Needs
These systems need strict traffic separation (flight-critical control vs. telemetry/sensor data vs. bulk/video/cloud traffic). They require bounded-latency channels, watchdog-aware comms, failover behaviors across multiple links (radio/Ethernet/Wi-Fi), and time sync.

### RT / Industrial Needs
Deterministic behavior, bounded jitter, hardware timestamping, low allocation pressure, and static routing/config are essential.

### Traffic Classes
Every communication channel must be tagged into classes (e.g., Class A: safety/control, Class B: real-time telemetry, Class C: diagnostics, Class D: bulk data, Class E: cloud/OTA) so the OS can apply per-profile policy to ensure infotainment cannot preempt safety classes.

## Profile Model Matrix

The following matrix anchors the roadmap. Features marked **baseline target** are the immediate goals, while others remain **future phases**.

| Profile | IP Stack | Bus Support | Gatewaying | QoS / Traffic Classes | Time Sync / TSN | Fast Path / Accel | Security Features | Current Maturity |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Minimal Embedded** | Baseline Target | Future | Not Implemented | Baseline Target | Future | Not Implemented | Baseline Target | Early Scaffolding |
| **Edge Gateway** | Baseline Target | Future | Future | Future | Future | Future | Future | Early Scaffolding |
| **Automotive** | Baseline Target | Future | Future | Future | Future | Not Implemented | Baseline Target | Early Scaffolding |
| **Drone / Robotics** | Baseline Target | Future | Future | Future | Future | Not Implemented | Baseline Target | Early Scaffolding |
| **Security Appliance** | Baseline Target | Not Implemented | Not Implemented | Future | Not Implemented | Future | Future | Early Scaffolding |
| **Cloud Node** | Baseline Target | Not Implemented | Not Implemented | Future | Not Implemented | Future | Future | Early Scaffolding |

## Repo Structure

To support this architecture, the repository is structured as follows. Note that some components are planned for future phases.

### Drivers (Current & Future)
* `drivers/net/` - Ethernet, Wi-Fi, virtio-net, etc.
* `drivers/bus/` - (Future) CAN, LIN, serial fieldbus, etc.

### Services
* `services/netstack/` - The minimum universal IP stack (ARP, IP, UDP, TCP).
* `services/netmgr/` - General network orchestration, IP config/routing.
* `services/net/` - **Legacy / Transitional.** The old monolithic stack. It remains for now so builds don't break but will be decomposed.
* `services/busmgr/` - (Future) Non-IP bus orchestration (CAN, LIN, serial), bus capability assignment.
* `services/gatewayd/` - (Future) Bridges, translates, and filters between domains (e.g., CAN ↔ IP).
* `services/time-syncd/` - (Future) Clock sync, PTP/gPTP, timestamp management.
* `services/netqosd/` - (Future) Traffic classes, priority policy.
* `services/safetyd/` - (Future) Communication safety policy, fail-closed/open decisions.

### Subsystems
* `subsys/network/` - General network contracts.
* `subsys/automotive/` - Automotive/transport communications contracts (CAN/LIN abstractions, gateway rules, domain separation). Future systems may generalize this to a broader `subsys/vehicle` abstraction.
* `subsys/rtcomms/` - (Future) Deterministic/real-time communications (traffic classes, priority contracts, deterministic queues).

### Libraries
* `lib/net/` - General IP types.
* `lib/packet/` - Packet/ring/buffer types.
* `lib/bus/` - (Future) Non-IP message buses, controller/channel abstractions, deterministic queue helpers.
* `lib/rtcomms/` - (Future) Traffic class definitions, bounded queue ops, scheduling hints.
* `lib/vehicle/` - (Future) Vehicle signal/message abstractions, gateway policy descriptors.

## Phase Roadmap

### Phase N1 — Universal Baseline (Current Scaffold)
* Scaffold `lib/net`, `lib/packet`, `subsys/network`, `services/netstack`, `services/netmgr`.
* Define early contracts, packet structures, and capability boundaries.
* Document future communications architecture spanning IP, control buses, and gatewaying.

### Phase N2 — Robustness (Future)
* Implement functional Ethernet, IPv4/v6, ARP, UDP, TCP.
* Basic routing, loopback, and DHCP.

### Phase N3 — Appliance/Cloud Acceleration (Future)
* Zero-copy packet path, flow steering, and `services/netfast`.

### Phase N4 — Vertical Profiles & Comms (Future)
* Subsystems for deterministic comms (`subsys/rtcomms`).
* Bus orchestration (`services/busmgr`, `lib/bus`).
* Automotive CAN/TSN gateways, domain translation (`services/gatewayd`).

## Dependency Rules
* The control plane (`netmgr`, future `busmgr`) configures state via capabilities.
* The data plane (`netstack`, future `netfast`, future `gatewayd`) executes fast forwarding.
* Drivers (`drivers/net/*`, future `drivers/bus/*`) own hardware interactions and offload reporting.
* **Crucial Rule:** Do not force all packets through one heavy user-space service path. The data plane must be per-core, lock-light, and zero-copy.

## Non-Goals for this Phase
* Full TCP/UDP implementations.
* Hardware-specific driver code (e.g., actual CAN drivers).
* Implementation of QoS, gateway bridging, or TSN logic.
* Deletion or destructive refactoring of `services/net`.
* Implementation of DPDK/XDP equivalents.
