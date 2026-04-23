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
version: 0.2
last_updated: 2026-04-22
tags:
  - ipc
  - urpc
  - multikernel
  - capability
  - architecture
  - profiles
---

# Bharat-OS ARC: IPC + uRPC + Multikernel Communication Architecture

## Contract Status — IPC/URPC Reliability

- **Spec:** ✅ Documented (this document, envelope, states, retry rules)
- **Implementation:** 🟡 Partial
  - L0 transport: implemented
  - L1 protocol: **scaffolded** (`mk_proto.c`) with **policy helpers (idempotency, retry classification, result→state mapping)** now present
  - Authorization: partial (stubbed in `mk_dispatch.c`)
- **Validation:** ❌ Missing
  - No stress tests for ordering, retries, backpressure

> NOTE: `mk_proto` now provides conservative, non-invasive helpers to classify operations (idempotent vs state-changing) and bound retries. Full transaction engine (ACK tracking, timeouts, duplicate suppression) remains to be implemented.

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

... (rest unchanged)
