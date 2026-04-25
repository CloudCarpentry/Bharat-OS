---
title: UAPI, IDL, and IPC Boundary
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - core
see_also:
  - README.md
---
# UAPI, IDL, and IPC Boundary

This document explicitly defines the separation of concerns between UAPI, IDL, and IPC layers in Bharat-OS. Maintaining strict discipline around these boundaries is critical for a stable ABI, decoupled service interactions, and long-term architectural health.

## Overview

```text
UAPI  → defines data structures (what data is)
IDL   → defines interactions (how components talk)
IPC   → implements transport (how messages move)
```

Or conceptually:

```text
[ UAPI structs ]
       ↓
[ IDL methods using those structs ]
       ↓
[ IPC transports those messages between endpoints ]
```

## Layer Definitions

### 1. UAPI (User/API boundary)

**Definition:** Stable, versioned, language-agnostic data contracts shared across the kernel, user-space services, and external consumers.

**Scope:** The `interface/uapi/` directory.

**Must Include:**
* C structs defining exact memory layout.
* Enums and constants (e.g., event kinds, error codes).
* ABI-stable types.
* Capability handles and standardized identifiers.

**Must NOT Include:**
* Function implementations.
* RPC method definitions or service logic.
* Transport-specific mechanisms (e.g., queue pointers).

### 2. IDL (Interface Definition Layer)

**Definition:** Service interface contracts that define *how components interact*, not *what the internal data structures look like*.

**Scope:** The `interface/idl/` directory (legacy alias: `interface/idl/` during migration).

**Must Include:**
* RPC methods (e.g., `Subscribe`, `Publish`, `ReadSnapshot`).
* Request and Response schemas that compose or reference UAPI types.
* Streaming/subscription service contracts.

**Must NOT Include:**
* Kernel-internal structures.
* Low-level ABI/memory layout assumptions.
* Transport or implementation-specific code.

### 3. IPC (Transport / Mechanism)

**Definition:** The runtime mechanism used to move messages defined by IDL, carrying data structures defined in UAPI.

**Scope:** `lib/ipc/`, `lib/urpc/`, or kernel IPC facilities.

**Must Include:**
* Message passing mechanics.
* Queue management, buffering, and synchronization.
* Transport-level framing.

**Must NOT Include:**
* Business logic or policy.
* Service-specific schema definitions.
* Enforcement of application-level invariants.

## Folder Placement Rules

* **`interface/uapi/`**: Contains stable shared contracts (e.g., `interface/uapi/bharat/system/telemetry.h`).
* **`interface/idl/`**: Contains service interface definitions (e.g., `interface/idl/telemetry_v1.bidl`).
* **`lib/ipc/` or `lib/urpc/`**: Implements transport mechanisms.
* **`core/services/`**: Contains service implementations that provide or consume IDL interfaces.
* **`core/kernel/`**: Emits or consumes UAPI types directly and leverages IPC, but typically does not participate in IDL-level RPC policies.

## DOs and DON'Ts

### DO
* **DO** define shared structures in UAPI.
* **DO** compose or reference UAPI types inside IDL messages.
* **DO** keep IDL entirely transport-agnostic.
* **DO** keep IPC mechanisms generic and strictly policy-free.

### DO NOT
* **DO NOT** duplicate data structures in IDL if they already exist (or should exist) in UAPI.
* **DO NOT** embed RPC method definitions or service routing in UAPI headers.
* **DO NOT** bake transport-specific (e.g., socket or queue) assumptions into IDL contracts.
* **DO NOT** allow services to invent private, incompatible ad-hoc schemas when a shared semantic concept exists.

## Concrete Examples

### Example 1: Telemetry Event

**UAPI (`interface/uapi/bharat/system/telemetry.h`):**
```c
typedef struct {
    uint64_t timestamp;
    uint32_t event_type;
    uint32_t severity;
    uint32_t source_id;
} bharat_event_header_t;
```

**IDL (`interface/idl/telemetry_v1.bidl`):**
```text
rpc Subscribe(SubscribeReq) -> SubscribeResp
rpc GetEventSnapshot(GetEventSnapshotReq) -> GetEventSnapshotResp
```

**IPC:**
* A message sent via a uRPC queue, carrying a serialized payload built upon the UAPI struct.

### Example 2: Telemetry Counter

**UAPI (`interface/uapi/bharat/system/telemetry.h`):**
```c
typedef struct {
    uint32_t counter_id;
    uint64_t value;
    uint32_t type; /* monotonic vs gauge */
} bharat_counter_t;
```

**IDL (`interface/idl/telemetry_v1.bidl`):**
```text
rpc ListCounters(ListCountersReq) -> ListCountersResp
rpc ReadCounterSnapshot(ReadCounterSnapshotReq) -> ReadCounterSnapshotResp
```

## Versioning Guidance

* **UAPI** requires extreme caution; changes can break the fundamental ABI contract between kernel and user-space or between separately compiled components. Backward compatibility is strictly required.
* **IDL** can evolve via versioned namespaces (e.g., `service.v1`, `service.v2`) allowing graceful service upgrades.
* **IPC** remains generic, evolving only to optimize or add new transport backends.
