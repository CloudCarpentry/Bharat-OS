# Edge Connectivity Profiles Architecture

## Overview

For Bharat-OS, a **trimmed, profile-driven connectivity stack** is crucial for edge devices. Instead of a monolithic desktop-style networking subsystem, Bharat-OS employs a **layered, selectable connectivity stack** where transports like Ethernet, Wi-Fi, BLE, Thread/802.15.4, CAN, UART, and even no-network mode are profile-selected.

This architecture is divided into three layers:
1.  **Core Network Substrate (`lib/net/`):** A small, stable layer providing packet buffers, link/address/route abstractions, eventing, timers, and basic protocol parsing (Ethernet, IPv4, UDP, etc.).
2.  **Optional Protocol Stacks (`services/network/netstack/`):** Loaded per profile. Features include IPv4, IPv6, UDP, TCP, DHCP client, DNS stub, mDNS, CoAP, MQTT, BLE host subset, etc.
3.  **Radio / Device Service Layer (`services/network/`):** Separate manager services like `netmgr`, `wifimgr`, `blemgr`, `meshmgr`, controlling policy, lifecycle, and connections.

## "Trimmed-Down" Definition

For Bharat-OS edge, a "trimmed-down" stack means:
*   Fewer protocols, runtime allocations, copies, and background daemons.
*   Smaller security surface.
*   Static limits and deterministic buffers.
*   Simpler power-aware reconnect logic.
*   Compile-time feature removal.
*   Optional control/data plane split.

## Connectivity Profiles

Bharat-OS uses explicit profiles to dictate network capabilities.

### `edge-net-min` (Wired Minimal Edge)
*Target: Industrial gateways, test rigs, appliance controllers.*
*   Ethernet only
*   IPv4, ARP, ICMP, UDP
*   DHCP, DNS stub
*   Static IP or DHCP
*   32 packet buffers
*   No TCP, no TLS, no Wi-Fi, no BLE

### `edge-wifi-lite` (Wi-Fi Edge)
*Target: Smart appliances, handheld edge nodes, consumer devices.*
*   Wi-Fi station mode
*   IPv4, DHCP, DNS
*   UDP + limited TCP
*   TLS client optional
*   Small connection table
*   No AP mode, no BLE

### `edge-ble-sensor` (BLE / Low-Power Edge)
*Target: Wearables, sensors, toys, proximity devices.*
*   BLE peripheral / GAP / selective GATT roles
*   No IP stack
*   Telemetry via BLE characteristics
*   Ultra-low-power timers

### `edge-converged` (Converged Edge Gateway)
*Target: High-end ARM/RISC-V edge hardware.*
*   Ethernet + Wi-Fi + BLE
*   IPv4/IPv6, TCP/UDP
*   DNS/DHCP/mDNS
*   MQTT or CoAP
*   OTA/update agent
*   Stronger observability

## Component Responsibilities

*   **Kernel:** IRQ delivery, DMA support, IOMMU hooks, capability checks for device access, shared memory primitives, timers, synchronization, low-level transport primitives.
*   **Services (`services/network/`):** Policy, interface lifecycle, scan/join/connect logic, IP assignment, routing policy, reconnect policy, BLE pairing policy, service discovery, telemetry.
*   **Drivers (`drivers/net/`, `drivers/wifi/`, `drivers/bluetooth/`):** NIC/Wi-Fi/BLE controller specifics, firmware loading, queue management, DMA rings, interrupt handling, power-state transitions.

## Wi-Fi and BLE Managers

These require distinct layers and should not be buried in a generic `netstack`.
*   **`wifimgr`:** Manages radio/firmware control, scan results, association/authentication, key management, link state, and packet TX/RX.
*   **`blemgr`:** Manages controller/HCI transport, host functions, GAP, GATT, security/pairing, attribute database, advertisement/scan scheduling, and power behavior.

## Power-Aware Design

Power awareness is mandatory for edge devices and is profile-controlled:
*   Radio duty-cycle hooks
*   Suspend/resume aware socket/session handling
*   BLE advertising interval policies
*   Wi-Fi reconnect backoff
*   Packet batching
*   Wake-on-radio hooks

## Security Tiers

Security scales with the profile:
*   **Tiny Profile:** Pre-shared credentials, static trust anchors, secure update, basic command authorization.
*   **Mid Edge Profile:** TLS client, pinned certs, BLE bonding with constrained policies, secure provisioning, signed update manifests.
*   **Rich Edge Profile:** mTLS, certificate rotation, remote policy updates, stronger audit/telemetry.

## Build-Time Feature Gating & Board Capability

Rely on feature gates, profile manifests, board capability descriptors, and static limits (e.g., max interfaces, max BLE peers, max sockets, max packet buffers).

---

## Roadmap

### Phase 1: Baseline Wired IPv4
*   **Protocols:** Ethernet, IPv4, ARP, ICMP, UDP, DHCP client, DNS stub, basic TCP.
*   **Implementation Target:** `lib/net/` core parsing, `netstack` runtime dataplane, `netmgr` basic policy.

### Phase 2: Wireless Integration
*   **Protocols:** Wi-Fi station management (`wifimgr`), BLE basic GAP/GATT subset (`blemgr`), MQTT client or CoAP, TLS client subset, mDNS (where justified).

### Phase 3: Advanced Capabilities
*   **Protocols:** IPv6, mesh/Thread, advanced BLE roles, firewall/QoS, AP mode, richer service discovery.
