# Bharat-OS `services/init` — Design Principles & Architecture

## 1. Purpose

This document defines the **correct role, scope, and architecture of `services/init`** in Bharat-OS.

The goal is to ensure:

- ❌ We **do NOT build a Linux/systemd-style init clone**
- ❌ We **do NOT over-engineer for tiny/RT devices**
- ✅ We **stay aligned with capability-based service architecture**
- ✅ We **support per-core kernel and future distributed kernel model**
- ✅ We **keep init small, deterministic, and disposable**

---

## 2. Core Philosophy

> **Init is NOT a permanent system manager.**
> **Init is a short-lived bootstrap coordinator.**

### Golden Rule

```
Kernel   = mechanism
Services = policy
Init     = bootstrap policy (temporary)
```

---

## 3. What Init IS

Init is a:

- Bootstrap orchestrator
- Profile-aware system initializer
- Dependency graph validator
- Readiness coordinator
- Boot health gate
- Handoff initiator

---

## 4. What Init is NOT

Init must NOT:

- ❌ Act as a permanent service supervisor
- ❌ Own process lifecycle (process_manager does)
- ❌ Own memory policy (vm_manager does)
- ❌ Own fault policy (faultmgr does)
- ❌ Own power/network/storage/UI logic
- ❌ Become a “god service”

---

## 5. Architectural Role in System

```
Kernel
  ↓
Init (bootstrap only)
  ↓
Core Services (namesvc, servicemgr, faultmgr)
  ↓
Service Managers (policy owners)
  ↓
Stacks / Personalities
```

Init **hands off authority early**.

---

## 6. Key Design Principles

### 6.1 Small by Default

- Minimal code footprint
- No heavy runtime logic
- Profile-dependent behavior

---

### 6.2 Deterministic Boot

- No unbounded retries
- No implicit dependency resolution
- Explicit ordering and validation

---

### 6.3 Capability-Aligned

- No bypass of capability model
- Services must register via system contracts
- Init coordinates, does NOT own execution model

---

### 6.4 Event-Driven (NOT loop-driven)

Bad:

```
for service in list:
    start()
    retry()
```

Good:

```
wait for events:
  - service_registered
  - service_ready
  - timeout
  - failure
```

---

### 6.5 Disposable by Design

Depending on profile:

- Init may:
  - idle
  - quiesce
  - or EXIT completely

---

## 7. Core Responsibilities

Init MUST:

1. Read boot context (from kernel)
2. Select system profile
3. Load and filter manifest
4. Validate dependency graph
5. Coordinate service readiness
6. Enforce boot deadlines
7. Classify boot outcome
8. Hand off to servicemgr/faultmgr

---

## 8. Boot Model (Corrected)

### NOT:

```
init → start A → start B → start C
```

### INSTEAD:

```
init:
  → validate graph
  → trigger bootstrap signals
  → wait for services to register
  → track readiness
  → enforce deadlines
  → classify state
  → handoff
```

---

## 9. Service Model Alignment

### ❗ Critical Rule

Services should **self-register**.

```
service:
  → start (platform/loader)
  → register with namesvc
  → signal ready

init:
  → observes and validates
```

Init does NOT fully control spawning.

---

## 10. Boot Classes (NEW)

Replace simple required/optional with:

```
BOOT_CLASS_CORE
BOOT_CLASS_INFRA
BOOT_CLASS_OPTIONAL
BOOT_CLASS_LATE
BOOT_CLASS_DIAGNOSTIC
```

### Behavior

| Class      | Init Responsibility           |
| ---------- | ----------------------------- |
| CORE       | strict enforcement            |
| INFRA      | required for system stability |
| OPTIONAL   | best-effort                   |
| LATE       | deferred                      |
| DIAGNOSTIC | conditional                   |

---

## 11. Boot State Machine

```
RESET
 → CONTEXT_READY
 → PROFILE_SELECTED
 → MANIFEST_SELECTED
 → GRAPH_VALIDATED
 → CORE_STARTING
 → CORE_READY
 → OPTIONAL_STARTING
 → HANDOFF_PREPARED
 → HANDOFF_COMPLETE
 → QUIESCENT / EXIT
```

### Failure Paths

```
REQUIRED_FAIL → SAFE_MODE
TIMEOUT       → SAFE_MODE
HANDOFF_FAIL  → DEGRADED_BOOT
```

---

## 12. Profile Behavior Matrix (Refined)

### Tiny (KB/MB devices)

- Static sequence only
- No dependency engine
- No retries
- No heavy IPC

```
init → run fixed steps → exit
```

---

### RT / Safety

- Deterministic boot
- Strict deadlines
- Fail-fast
- Immediate safe-mode

---

### Embedded Rich

- Minimal graph
- Bounded retries
- Early handoff

---

### Mobile / Desktop

- Staged boot
- Background services
- Diagnostics allowed
- Still no init bloat

---

### Cloud / Appliance

- Only infra readiness enforced
- Do NOT wait for all services
- Delegate orchestration early

---

## 13. Internal Architecture

```
init/
  core/
    boot_context
    manifest_engine
    dependency_validator
    event_loop
    health_gate
    handoff

  profiles/
    tiny
    rt
    embedded
    mobile
    desktop
```

---

## 14. Key Data Contracts

### Boot Context

```c
typedef struct {
    uint32_t profile;
    uint64_t capability_mask;
    uint32_t platform_id;
    bool safe_mode_requested;
} init_boot_context_t;
```

---

### Service Descriptor

```c
typedef struct {
    const char *name;
    uint32_t boot_class;
    uint32_t dependencies;
    uint32_t deadline_ms;
} init_service_desc_t;
```

---

### Handoff Summary

```c
typedef struct {
    uint32_t boot_phase;
    uint32_t required_ok;
    uint32_t required_failed;
    uint32_t failure_class;
} init_handoff_summary_t;
```

---

## 15. Failure Classification

```
INIT_FAIL_DEP
INIT_FAIL_TIMEOUT
INIT_FAIL_LAUNCH
INIT_FAIL_CAPABILITY
INIT_FAIL_PROFILE
INIT_FAIL_HEARTBEAT
```

---

## 16. Handoff Protocol (MANDATORY)

Init MUST send structured data to:

- servicemgr
- faultmgr

```
INIT → servicemgr:
  - boot graph
  - service status
  - failure summary
  - profile
```

And receive ACK.

---

## 17. Guardrails (VERY IMPORTANT)

To prevent architecture drift:

- Init must remain < small bounded size
- No feature creep into init
- No policy logic beyond bootstrap
- No long-lived loops owning system behavior
- All extensions go to:
  - servicemgr
  - faultmgr
  - other managers

---

## 18. Definition of Done

Init is correct when:

- ✅ Bootstrap-only authority
- ✅ Profile-aware
- ✅ Deterministic startup
- ✅ Critical health gating
- ✅ Structured handoff
- ✅ Works on tiny → cloud systems
- ✅ No Linux/systemd-style bloat

---

## 19. Final Statement

> **Init is a short-lived, profile-aware bootstrap coordinator that validates system intent, ensures critical readiness, and transfers lifecycle authority.**

Nothing more.

---

## 20. Future Compatibility

This design:

- Supports per-core kernel ownership
- Works with distributed kernel (multi-node)
- Aligns with capability-based IPC
- Scales from KB devices to cloud systems

---
