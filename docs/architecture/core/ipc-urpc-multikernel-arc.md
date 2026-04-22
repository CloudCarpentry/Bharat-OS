---
title: IPC + uRPC + Multikernel Communication Architecture
status: Proposed
see_also:
  - ../contracts/naming-conventions.md
  - service-identity-and-incarnation.md
owner: Kernel Architecture
reviewers:
  - Core Kernel Team
  - IPC / Multikernel Maintainers
version: 0.1
last_updated: 2026-03-23
tags:
  - ipc
  - urpc
  - multikernel
  - capability
  - architecture
  - profiles
---

# Bharat-OS ARC: IPC + uRPC + Multikernel Communication Architecture

## 1. 🎯 Purpose

This document defines the unified communication architecture for Bharat-OS, covering:

- Local IPC (endpoint-based)
- Async IPC (request-driven)
- Cross-core communication (uRPC)
- Distributed object ownership (multikernel model)
- Capability-safe delegation
- Profile-aware behavior (RT / GP / MIX)
- Future integration with AI Governor

## 2. 🧠 Architectural Vision

Bharat-OS is not just a microkernel — it is a:

**Capability-governed, profile-driven, multikernel OS with communication as the core primitive**

All system interactions must follow:

`Compute = Message + Capability + Policy`

## 3. 🏗️ Layered Communication Model

### 🔹 L0 — Transport Layer (uRPC)
- Per-core ring buffers (SPSC)
- Cache-friendly, lock-free
- Memory ordering (acquire/release)
- IPI or event-based wakeup

👉 **Files:**
- `kernel/src/ipc/multikernel.c`
- `kernel/include/advanced/multikernel.h`

### 🔹 L1 — Delivery Layer (Protocol Engine) 🟡 Partial
Responsible for:
- ACK / NACK (Implemented)
- Timeout handling (Implemented)
- Retry policy (Policy helpers implemented)
- Duplicate suppression (Pending)
- Transaction lifecycle (Partial: Begin, Complete, Poll Timeouts)

👉 **To be implemented in:**
- `kernel/src/ipc/mk_proto.c`
👉 **To be implemented in:**
- `kernel/src/ipc/mk_proto.c`

### 🔹 L2 — Object RPC Layer
Handles:
- Frame operations
- Address space operations
- Process control
- Capability delegation

👉 **Uses:**
- Typed messages (`MK_MSG_*`)
- Ownership-aware routing

### 🔹 L3 — Service IPC Layer
System services:
- namesvc
- console
- netmgr
- device manager

👉 **Built on top of L2 + endpoint IPC**

## 3.1 🛡️ uRPC Security Model

**uRPC is a distributed security and execution boundary, not just a transport.**

- uRPC frames are part of the authority path.
- Every cross-core operation MUST bind transport identity, capability identity, and revocation epoch.
- No cross-core operation is valid solely because it reached the destination ring.

## 3.2 ✉️ Canonical uRPC Message Envelope

Every message MUST follow this stable contract:

```c
typedef struct urpc_msg_hdr {
    uint64_t tx_id;              // unique per sender
    uint32_t src_core;
    uint32_t dst_core;

    uint64_t service_id;         // logical service identity
    uint32_t endpoint_id;        // logical endpoint
    uint32_t endpoint_gen;       // incarnation (restart-safe)

    uint32_t opcode;
    uint32_t flags;

    uint64_t capability_id;      // authority binding
    uint64_t capability_epoch;   // revocation epoch

    uint32_t payload_len;
    uint32_t hdr_crc;
} urpc_msg_hdr_t;
```

**Enforced Guarantees:**
- Stale endpoint use → Rejected (`endpoint_gen` check)
- Revoked capability use → Rejected (`capability_epoch` check)
- Replay / Duplicate → Rejected (`tx_id` + epoch)
- Cross-core drift → Bounded

## 3.3 🔄 Protocol Engine Semantics (L1)

The missing L1 protocol engine must formally implement the following states and behaviors:

### Transaction States
- `SENT`
- `DELIVERED`
- `ACKED`
- `COMPLETED`
- `FAILED_TIMEOUT`
- `FAILED_REVOKED`
- `FAILED_STALE_ENDPOINT`

### Responsibilities
- **ACK/NACK:** Mandatory for all authoritative operations.
- **Timeout Classes:** Fast (RT), Slow (GP).
- **Duplicate Suppression:** Via `tx_id` tracking.
- **Transaction Lifecycle:** Bounded completion (no infinite in-flight operations).

## 3.4 🔁 Retry and Idempotency Rules

**Hard Rules:**
- Retry allowed ONLY for read-only or explicitly idempotent operations.
- **NEVER** auto-retry:
  - Capability grant
  - Capability revoke
  - DMA map
  - Device queue submit
  - State-changing admin operations

| Operation Type    | Retry Allowed |
|-------------------|---------------|
| Read-only         | ✅            |
| Stateless         | ✅            |
| Device submission | ❌            |
| Capability grant  | ❌            |
| DMA map           | ❌            |

## 3.5 🆔 Endpoint / Service Incarnation Model

To prevent stale endpoint reuse after a service restart:

- `service_id` is stable across restarts.
- `endpoint_gen` (incarnation generation) MUST increment on restart.
- Old in-flight messages sent to prior incarnations MUST fail closed.
- `namesvc` MUST return the latest incarnation.

## 3.6 💥 Cross-Core Failure Semantics

Explicit outcomes for distributed failure scenarios:

| Scenario | Behavior |
|----------|----------|
| Sender crash before ACK | Pending transaction invalidated / rolled back |
| Receiver crash after side effect, before ACK | Receiver `endpoint_gen++` on recovery; sender must reconcile |
| Revoke racing with invoke | Invoke fails with `ERR_CAP_REVOKED` |
| Remote service restart during transaction | Fail closed (stale generation) |

## 3.7 🚦 Profile-Aware Transport Policy

Transport behavior must adapt to the system profile:

- **RT (Real-Time):** Fixed ring sizes, no dynamic allocation, NO transport retries, bounded in-flight slots.
- **GP (General Purpose):** Larger buffers, retries allowed on idempotent classes.
- **MIX (Mixed-Criticality):** Strict separation between control-plane (GP rules) and data-plane (RT rules) channels.

## 4. 🔄 Communication Types

### 4.1 Endpoint IPC (Local)
- Synchronous
- Capability-controlled
- Blocking semantics
- Bounded payload

👉 **Strength:** Mature

### 4.2 Async IPC (Local)
- Request-based
- Timeout-aware
- Non-blocking
- Currently global table (⚠️ issue)

### 4.3 uRPC (Cross-Core)
- Asynchronous transport
- Ring-buffer based
- Protocol L1 (ACK, Timeouts, Identity) required

### 4.4 Remote Endpoint (Future)
- Local API → remote execution
- Transparent cross-core messaging

## 5. 🔐 Capability Model (Core Principle)

All communication must be:

**Authorized + Validated + Ownership-aware**

Required Rules:
- Sender must possess capability
- Destination core validates ownership
- Rights attenuation enforced
- Remote delegation must:
  - succeed fully OR
  - rollback safely

## 6. ⚠️ Current Gaps (Critical Analysis)

| Area | Issue | Impact |
|------|-------|--------|
| **Protocol Layer** | Missing (`mk_proto` incomplete) | No reliability |
| **Authorization** | Stubbed in `mk_dispatch` | Security risk |
| **Async IPC** | Global table | Scalability bottleneck |
| **Docs vs Code** | Mismatch | Developer confusion |
| **Service Integration** | Partial | IPC not exercised |
| **Endpoint ↔ uRPC bridge** | Missing | No unified model |

## 7. 🎯 Target State

A fully unified communication model:

```
Local IPC
   ↓
Async IPC Layer
   ↓
Protocol Engine (L1)
   ↓
uRPC Transport (L0)
   ↓
Remote Core
```

## 8. ⚙️ Profile-Aware Behavior

### 🔸 RT Profile
- No dynamic allocation
- No retries
- Fixed-size rings
- Deterministic latency

### 🔸 GP Profile
- Large buffers
- Retries allowed
- Rich async support

### 🔸 MIX Profile
- Control-plane → GP
- Data-plane → RT

## 9. 🤖 AI Governor Integration (Forward-Looking)

Optional policy layer:
- Dynamic routing (local vs remote)
- Adaptive retry/backoff
- QoS tuning
- Load-aware core selection
- Thermal-aware scheduling

👉 **Must be:**
- Optional
- Pluggable
- Non-blocking

## 10. 🧩 Architecture Diagram (Conceptual)

```
+----------------------+
|   Service Layer (L3) |
+----------------------+
          ↓
+----------------------+
|   Object RPC (L2)    |
+----------------------+
          ↓
+----------------------+
| Protocol Engine (L1) |
+----------------------+
          ↓
+----------------------+
| uRPC Transport (L0)  |
+----------------------+
          ↓
+----------------------+
|   Remote Core        |
+----------------------+
```

## 11. 🚀 Implementation Roadmap

### 🟢 Phase 0 — Reality Alignment
**Goal:** Fix truth mismatch
**Tasks:**
- Update IPC docs to match code
- Correct maturity status
- Standardize terminology

### 🟡 Phase 1 — Protocol Engine (MOST IMPORTANT)
**Goal:** Make uRPC usable
**Tasks:**
- Transaction table (per-core)
- ACK/NACK handling
- Timeout engine
- Retry policy (idempotent only)
- Channel state transitions
**Files:** `mk_proto.c`, `mk_dispatch.c`

### 🟠 Phase 2 — Capability-Safe Messaging
**Tasks:**
- Replace stub authorization
- Capability validation in dispatch
- Remote delegation support
- Rollback handling

### 🔵 Phase 3 — Performance + Scalability
**Tasks:**
- Replace global async table
- Per-core request pools
- QoS propagation
- Zero-copy optimizations (future)

### 🟣 Phase 4 — Service Integration
**Tasks:**
- Convert console to uRPC
- Convert namesvc
- Add cross-core ownership RPC

### ⚫ Phase 5 — Advanced Features
**Tasks:**
- Remote endpoint abstraction
- Distributed scheduling
- AI Governor hooks

## 12. 📌 Immediate Tickets (Actionable)

- **🎫 Ticket 1 — ARC Document (this file)**: Added to repo.
- **🎫 Ticket 2 — Fix Docs Drift**: Update `IPC_ARCHITECTURE.md`, align paths and terminology.
- **🎫 Ticket 3 — Transaction Table**: Add inflight tracking, complete ACK/NACK path.
- **🎫 Ticket 4 — Authorization Layer**: Replace stub in `mk_dispatch.c`, add capability validation.
- **🎫 Ticket 5 — Async IPC Refactor**: Replace global table, introduce per-core pools.

## 13. 🧠 Design Principles

- Message-first design
- Capability-enforced security
- Per-core ownership (multikernel)
- Determinism for RT
- Async-first, sync-as-wrapper
- Minimal kernel, rich services
- Policy-driven behavior

## 14. 🔥 Key Insight (Critical)

uRPC today is a fast mailbox.
**It must become a reliable distributed protocol.**

This is the single most important transformation.

## 15. 📍 Final Recommendation

Do NOT expand features randomly.
Focus only on:
**Protocol Engine + Capability Validation + Transaction Lifecycle**
Everything else will naturally align after that.
