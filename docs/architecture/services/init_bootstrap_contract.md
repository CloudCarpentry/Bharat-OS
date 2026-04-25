# Bharat-OS `core/services/init` — Design Principles & Architecture

## 1. Purpose

This document defines the **correct role, scope, and architecture of `core/services/init`** in Bharat-OS.

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
  → start (core/platform/loader)
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

# 21. Kernel Self-Tests vs Init Boot GatingA large part of Bharat-OS correctness today comes from low-level kernel self-tests and bring-up validation such as:- PMM self-tests- VMM self-tests- page-table / TLB checks- trap and fault-path checks- discovery / FDT / platform parsing checks- timer / IRQ / early console sanity- low-level IPC and capability primitive checksThese mechanisms are essential, but they do **NOT** belong inside `core/services/init`.### Core Rule```textKernel runs mechanism self-tests.Init consumes summarized results and makes boot policy decisions.Servicemgr / faultmgr own long-term lifecycle and runtime recovery.

22. Responsibility Split
    22.1 What stays in Kernel
    The kernel remains responsible for running and owning:

PMM/VMM self-tests

memory and mapping invariant checks

architecture/platform bring-up checks

trap/fault mechanism tests

discovery parser validation

low-level IPC primitive tests

capability primitive tests

timer / interrupt / early console sanity tests

These are mechanism-level correctness checks and must stay in the kernel.
Init must not duplicate or re-implement them.

22.2 What Init Does With Kernel Results
Init acts as a bootstrap health gate.
Init should:

read a structured kernel boot/self-test summary

classify whether the system may continue boot

decide between:

normal boot

degraded boot

safe mode

diagnostic mode

use profile policy to interpret failures

continue or stop service bootstrap accordingly

So init is not the test runner.
Init is the consumer of kernel health state.

22.3 What Belongs to Service Readiness
After kernel self-tests pass sufficiently, init then coordinates service-layer readiness such as:

namesvc registered and reachable

servicemgr ready to accept handoff

faultmgr reachable if required by profile

process_manager registered if part of required graph

vm_manager registered if part of required graph

These are service bootstrap readiness checks, and they do belong in init.

23. Three-Phase Boot Model
    Phase A — Kernel Bring-up and Kernel Self-Test
    Kernel performs:

architecture/platform initialization

discovery and hardware capability resolution

PMM/VMM initialization

kernel invariant self-tests

export of a structured boot health summary

At this stage, init does not own policy yet.

Phase B — Init Boot Gate
Init reads:

kernel boot context

kernel self-test summary

selected profile

platform / board / personality metadata

safe-mode / diagnostics request

Init then decides:

continue normal bootstrap

continue degraded bootstrap

switch to safe mode

switch to diagnostic mode

halt further bootstrap

Phase C — Service Graph Convergence
Init coordinates:

required bootstrap services

service registration

readiness tracking

deadlines

handoff to servicemgr / faultmgr

This preserves clean separation between mechanism correctness and service orchestration.

24. Required Kernel-to-Init Contract
    To support this model, the system should define a structured kernel boot health summary passed to init.
    Example shape:
    typedef enum { KBOOT_TEST_NOT_RUN = 0, KBOOT_TEST_PASS, KBOOT_TEST_WARN, KBOOT_TEST_FAIL,} kboot_test_result_t;typedef struct { kboot_test_result_t pmm; kboot_test_result_t vmm; kboot_test_result_t discovery; kboot_test_result_t ipc; kboot_test_result_t traps; kboot_test_result_t timer; uint64_t capability_mask; uint64_t hw_feature_mask; bool degraded_allowed;} kboot_health_summary_t;
    This allows init to stay small while still making profile-aware decisions.

25. Profile-Aware Interpretation of Kernel Health
    Kernel self-test results must be interpreted differently depending on profile.
    Tiny

minimal, bounded self-tests only

init reads a compact pass/fail summary

fail-fast if essential kernel invariants are broken

RT / Safety

strict interpretation of PMM/VMM/trap/timer failures

immediate safe-mode or stop on critical kernel failure

warnings must remain tightly bounded and policy-controlled

Embedded Rich

allows some degraded continuation depending on failure class

still blocks boot on core memory or trap failures

Mobile / Desktop

may allow degraded continuation for some non-core facilities

must still block on essential kernel correctness failures

Cloud / Appliance

core infrastructure correctness remains mandatory

some optional diagnostics or non-essential health warnings may not block infra bring-up

26. What Must NOT Happen
    Bad Pattern 1 — Re-running kernel self-tests in init
    Init must not embed PMM/VMM/discovery/timer/trap self-test logic.
    Why this is wrong:

duplicates mechanism logic

creates drift between kernel truth and init truth

increases tiny/RT footprint

violates core/kernel/service boundary

Bad Pattern 2 — Kernel directly owning service bootstrap policy
Kernel self-tests must not expand into service-graph policy ownership.
Why this is wrong:

kernel should remain mechanism-only

service orchestration belongs above kernel

it weakens the service-oriented architecture

27. Final Boundary Rule
    Kernel = self-test execution + mechanism truthInit = boot gating + service bootstrap coordinationServicemgr = long-lived lifecycle authorityFaultmgr = runtime fault/degradation policy
    This boundary is mandatory for keeping Bharat-OS:

capability-aligned

profile-aware

suitable for tiny devices

compatible with per-core kernel ownership

ready for future distributed-kernel evolution
