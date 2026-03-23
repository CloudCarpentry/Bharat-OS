---
title: IPC + uRPC + Multikernel Communication Architecture
status: Proposed
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

### 🔹 L1 — Delivery Layer (Protocol Engine) ❗ Missing (Critical Gap)
Responsible for:
- ACK / NACK
- Timeout handling
- Retry policy
- Duplicate suppression
- Transaction lifecycle

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
- No full protocol yet (⚠️ major gap)

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
