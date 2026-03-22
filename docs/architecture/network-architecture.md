# Bharat-OS Network & Communications Architecture

## 1. Overview

Networking in Bharat-OS is evolving from a monolithic service into a **modular, profile-driven communications framework**.

This document defines:
* current state
* target architecture
* repository mapping
* future roadmap

---

## 2. Current State

Today, networking exists in multiple partially overlapping forms:
* `services/net` (monolithic, legacy)
* `services/netmgr` (control-plane direction)
* `services/netstack` (data-plane direction)
* `lib/net`, `lib/packet` (shared abstractions)

Limitations:
* unclear separation of responsibilities
* no formal subsystem contract
* no support for non-IP communication (CAN, etc.)
* no QoS or real-time model

---

## 3. Target Architecture

### 3.1 Layered Model

Bharat-OS networking is structured into:

#### Control Plane
* interface lifecycle
* routing
* DHCP / configuration
* policy management

→ `services/netmgr`

#### Data Plane
* packet processing
* TCP/IP stack
* ARP/NDP
* UDP/TCP

→ `services/netstack`

#### Shared Libraries
* common types → `lib/net`
* packet buffers → `lib/packet`

#### Subsystem Layer
* system-wide contracts
* profile abstraction

→ `stacks/network`

---

## 4. Networking vs Communications

Bharat-OS will evolve beyond traditional networking into a full **communications framework**.

### Communication Families

| Type                   | Examples     |
| ---------------------- | ------------ |
| Control Buses          | CAN, LIN     |
| Deterministic Ethernet | TSN          |
| General Networking     | TCP/IP       |
| Wireless               | Wi-Fi, radio |

---

## 5. Repository Mapping

| Layer         | Component         |
| ------------- | ----------------- |
| Subsystem     | stacks/network    |
| Control Plane | services/netmgr   |
| Data Plane    | services/netstack |
| Legacy        | services/net      |
| Shared Types  | lib/net           |
| Packet Model  | lib/packet        |

---

## 6. Future Phases

### Phase N2
* bus manager (CAN/LIN)
* gateway service
* time synchronization
* QoS layer

### Phase N3
* fast dataplane (zero-copy)
* hardware offload
* multi-queue NIC support

### Phase N4
* automotive / drone / RT communication models
* TSN
* deterministic scheduling

---

## 7. Profile Matrix

| Profile    | IP       | Bus        | QoS        | Gateway | Fast Path |
| ---------- | -------- | ---------- | ---------- | ------- | --------- |
| Embedded   | basic    | future     | minimal    | no      | no        |
| Automotive | yes      | CAN/LIN    | strict     | yes     | limited   |
| Drone      | yes      | CAN/UAVCAN | priority   | yes     | partial   |
| Appliance  | advanced | optional   | strong     | yes     | yes       |
| Cloud      | advanced | no         | high-scale | yes     | yes       |

---

## 8. Transitional Note

`services/net` is considered **legacy** and will be decomposed into:
* `netmgr` (control plane)
* `netstack` (data plane)

---

## 9. Status

**Status: Architecture Defined (Implementation Pending)**
