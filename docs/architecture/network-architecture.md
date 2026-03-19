# Bharat-OS Network Architecture

This document defines the capability-safe, profile-driven networking and communications architecture for Bharat-OS. It outlines the strategic move from a basic network placeholder to a first-class subsystem designed for multiple device classes (embedded, edge gateways, automotive, robotics, and cloud nodes).

## Why Networking is a First-Class Subsystem

Networking in Bharat-OS is not merely a set of drivers. It is a fundamental operational plane that requires:
* Capability-mediated access control (between applications, stacks, and NICs).
* Deterministic behavior for RTOS/Automotive profiles.
* High throughput and zero-copy semantics for Edge Gateway and Cloud Node profiles.
* Clean separation of control policy from the fast data path.

Because networking spans across these deeply divergent constraints, it is elevated to a first-class subsystem (`subsys/network`), comparable to the Memory Management and Subsystems domains.

## Why `services/net` is No Longer Enough

The original `services/net` directory acted as a monolithic blob that attempted to hold drivers, stacks, and routing policy in one place. A monolithic user-space network service fails to scale:
* **Performance:** Forcing all packet processing (control, forwarding, routing) through a single user-space domain introduces unacceptable IPC overhead and locking contention.
* **Security & Reliability:** A single crash in a generic driver or protocol parser would take down the entire network stack.
* **Profile Flexibility:** Embedded devices cannot afford a monolithic TCP/IP stack, while Edge Gateways require high-speed dataplane logic (like XDP or DPDK) completely separated from configuration agents.

To solve this, `services/net` is being explicitly decomposed into a multi-layered structure.

## Layered Model

The Bharat-OS network architecture is split into three primary layers, ensuring that policy, basic connectivity, and high-speed data flow remain isolated and configurable.

### 1. Minimum Universal Core
This is the **always-on baseline** for any build requiring networking.
* **Ownership:** `services/netstack` and `lib/net`
* **Responsibilities:**
  * NIC discovery and link state.
  * Ethernet, VLAN, ARP/NDP.
  * IPv4 and IPv6.
  * ICMP/ICMPv6, UDP, basic TCP.
  * Socket-like API contracts.
  * Basic routing and DHCP client operations.

### 2. Profile-Specific Feature Planes
Above the baseline, extra capabilities are activated based on the build profile (e.g., Automotive vs. Cloud).
* **Ownership:** `services/netmgr`, `subsys/network`, and targeted services (e.g., future `services/netsec`).
* **Responsibilities:**
  * Stateful firewalls and NAT (Security Appliances).
  * Policy routing and VRF (Cloud Nodes/Gateways).
  * Multiqueue NIC scaling (Cloud Nodes).
  * Power-aware networking (Mobile/Edge).

### 3. Fast Path / Acceleration Plane (Roadmap)
A dedicated lane for line-rate packet processing that bypasses the control plane entirely.
* **Ownership:** Future `services/netfast`
* **Responsibilities:**
  * Zero-copy RX/TX rings (`lib/packet`).
  * Lock-minimized, per-core queues.
  * XDP-like early packet drop/steering.
  * Hardware accelerator / NIC DMA integration.

## Networking vs Communications

While Bharat-OS networking starts with an IP networking core, the long-term architecture must also support bus-oriented, deterministic, and industrial communications.

* **Current Focus (Phase N1):** IP-based Ethernet connectivity (`subsys/network`, `services/netstack`).
* **Future Evolution:**
  * `subsys/vehicle` and `subsys/rtcomms`
  * `lib/bus`
  * `services/busmgr` (CAN/LIN/FlexRay/SOME/IP)
  * `services/gatewayd` (Bridging distinct comms domains)
  * `services/time-syncd` (PTP/TSN orchestration)
  * `services/netqosd` (Traffic shaping and deterministic queues)

## Profile Model Matrix

The following matrix anchors the roadmap. Features marked **baseline target** are the immediate goals, while others remain **future phases**.

| Profile | IP Stack | Bus Support | Gatewaying | QoS / Traffic Classes | Fast Path / Acceleration | Overlay / Tunnels | TSN / Time Sync | Security Features |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Minimal Embedded** | Baseline Target | Future | Not Implemented | Baseline Target | Not Implemented | Not Implemented | Future | Baseline Target |
| **Edge Gateway** | Baseline Target | Future | Future | Future | Future | Future | Future | Future |
| **Automotive** | Baseline Target | Future | Future | Future | Not Implemented | Not Implemented | Future | Baseline Target |
| **Drone / Robotics** | Baseline Target | Future | Future | Future | Not Implemented | Not Implemented | Future | Baseline Target |
| **Security Appliance** | Baseline Target | Not Implemented | Not Implemented | Future | Future | Future | Not Implemented | Future |
| **Cloud Node** | Baseline Target | Not Implemented | Not Implemented | Future | Future | Future | Not Implemented | Future |

## Repo Structure

To support this architecture, the repository is structured as follows:

* `subsys/network/` - The network subsystem contract layer (native API contracts, profile feature flags).
* `lib/net/` - Common network definitions (interface IDs, address families).
* `lib/packet/` - Packet buffer descriptors, ring abstractions, and headroom/tailroom flags.
* `services/netstack/` - The minimum universal IP stack (ARP, IP, UDP, TCP).
* `services/netmgr/` - Orchestration, interface lifecycle, policy distribution, and routing table ownership.
* `services/net/` - **Legacy / Transitional.** The old monolithic stack. It remains for now so builds don't break but will be decomposed.

## Phase Roadmap

### Phase N1 — Universal Baseline (Current Scaffold)
* Scaffold `lib/net`, `lib/packet`, `subsys/network`, `services/netstack`, `services/netmgr`.
* Define early contracts, packet structures, and capability boundaries.

### Phase N2 — Robustness (Future)
* Implement functional Ethernet, IPv4/v6, ARP, UDP, TCP.
* Basic routing, loopback, and DHCP.

### Phase N3 — Appliance/Cloud Acceleration (Future)
* Zero-copy packet path, flow steering, and `services/netfast`.

### Phase N4 — Vertical Profiles & Comms (Future)
* Automotive CAN/TSN gateways, edge/mobile power states, `services/busmgr`.

## Dependency Rules
* The control plane (`netmgr`) configures state via capabilities.
* The data plane (`netstack`, `netfast`) executes fast forwarding.
* Drivers (`drivers/net/*`) own hardware interactions and offload reporting.
* **Crucial Rule:** Do not force all packets through one heavy user-space service path. The data plane must be per-core, lock-light, and zero-copy.

## Non-Goals for this Phase
* Full TCP/UDP implementations.
* Hardware-specific driver code.
* Deletion or destructive refactoring of `services/net`.
* Implementation of DPDK/XDP equivalents.
