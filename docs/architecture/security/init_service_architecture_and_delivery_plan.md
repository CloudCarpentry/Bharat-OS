# Bharat-OS `core/services/init` Deep Analysis and Implementation Plan

## 1) Executive Summary

`core/services/core/init` already follows the right **directional idea** (manifest filtering, profile gating, dependency-aware start order, and supervisor handoff hook), but it is still a scaffold-level bootstrap with stub launch functions and compile-time simulated context.

The correct next step is **not** to turn init into a permanent god-process. Instead, Bharat-OS should keep init as a **small, profile-aware bootstrap policy orchestrator** that:

1. Builds the minimal valid early runtime graph,
2. Performs critical-boot health gating, and
3. Hands long-term lifecycle responsibility to `servicemgr` / `faultmgr`.

This document provides:
- a code-grounded gap analysis,
- a concrete target architecture,
- a phased implementation plan,
- file/module layout and C contract sketches,
- profile behavior matrix,
- validation and rollout criteria.

---

## 2) Current Code Reality (Deep Analysis)

## 2.1 What already exists and is good

### A. Bootstrap flow exists in `core/services/core/init`
- `init_main.c` initializes runtime, reads bootstrap capability, builds boot context, runs runtime graph, and idles.
- This already establishes init as first userspace coordinator.

### B. Static manifest model exists
- `init_manifest.h` defines service descriptors with:
  - profile mask,
  - capability requirements,
  - dependency list,
  - required/optional policy,
  - retry limits.
- `init_manifest.c` defines a compile-time graph with core services (`namesvc`, `process_manager`, `vm_manager`, `servicemgr`, `faultmgr`, etc.).

### C. Deterministic startup loop exists
- `init_runtime.c`:
  - filters manifest entries against context,
  - checks dependencies,
  - attempts bounded retries,
  - fails boot on required service failure,
  - calls handoff hook.

### D. Boundary intent is documented
- Existing architecture docs already state kernel=mechanism and services=policy, and position init as bootstrap rather than full lifecycle owner.

## 2.2 Key gaps (what prevents production readiness)

### A. Launch path is still stubbed
- Manifest start handlers are currently `stub_start` placeholders.
- No real spawn/activate/registration handshake with actual service binaries.

### B. Boot context is mostly compile-time/defaulted
- Profile detection and capability set are macro-driven placeholders.
- Board/core/platform/personality discovery is not sourced from a kernel boot contract yet.

### C. Dependency model is minimal and not externally introspectable
- In-memory only; no boot graph export contract for diagnostics tooling.
- No topological pre-validation (cycle detection, deterministic tie-break rules) as a first-class explicit stage.

### D. Health gate is coarse
- Required service failure triggers safe-mode flag, but no structured failure classes/reasons/codes that higher-level fault policy can consume reliably.

### E. Handoff is a hook, not a protocol
- `init_handoff_to_supervisor` logs intention but does not perform a typed IPC handover contract with `servicemgr` / `faultmgr`.

### F. Role overlap risk in docs/readmes
- Some service READMEs still describe responsibilities ambiguously (e.g., naming/discovery vs supervision boundaries), which can lead to architecture drift if implementation starts expanding init scope.

---

## 3) Target Architecture (What init should be in Bharat-OS)

## 3.1 Core principle

`init` should be a **bootstrap policy engine with intentionally short authority**.

- It must **own early boot orchestration**.
- It must **not own long-lived supervision policy**.
- After early boot convergence, authority transfers to dedicated managers.

## 3.2 Responsibility boundaries

### `init` MUST own
- boot context intake and normalization,
- profile selection,
- manifest loading/selection,
- deterministic startup ordering,
- required vs optional classification enforcement,
- critical boot timeout/deadline checks,
- boot phase publication,
- structured handoff packet to supervisor chain.

### `init` MUST NOT own
- indefinite service supervision loops,
- deep process tree ownership (belongs to `process_manager`),
- address-space policy (belongs to `vm_manager`),
- fault domain strategy (belongs to `faultmgr`),
- network/storage/power/UI policy orchestration.

## 3.3 Layered init model

### Layer A: Universal Init Core (always present)
- boot context parser,
- profile resolver,
- manifest engine,
- dependency scheduler,
- launch adapter API,
- failure classifier,
- boot state publisher,
- handoff client.

### Layer B: Profile adapters (policy variation only)
- tiny,
- embedded_rich,
- rt/safety,
- mobile,
- desktop,
- cloud/appliance (future).

### Layer C: Post-bootstrap ownership targets
- `servicemgr` (service lifecycle monitoring),
- `faultmgr` (fault policy and degradation),
- `namesvc` (registration/discovery),
- `telemetrymgr` (boot event export),
- power/network/storage managers (profile dependent).

---

## 4) Proposed Module/File Architecture

Use one engine + profile modules (avoid giant preprocessor branching):

```text
core/services/core/init/
  include/
    init_contract.h
    init_boot_context.h
    init_manifest.h
    init_dependency.h
    init_launch.h
    init_health_gate.h
    init_handoff.h
    init_profile.h
  src/
    init_main.c
    init_boot_context.c
    init_manifest.c
    init_dependency.c
    init_launch.c
    init_health_gate.c
    init_handoff.c
    init_status.c
    profiles/
      init_profile_tiny.c
      init_profile_embedded_rich.c
      init_profile_rt.c
      init_profile_mobile.c
      init_profile_desktop.c
```

> Note: Existing files can be incrementally migrated to this layout; no big-bang rewrite required.

---

## 5) Boot State Machine (Normative)

```text
RESET
  -> CONTEXT_READY
  -> PROFILE_SELECTED
  -> MANIFEST_SELECTED
  -> GRAPH_VALIDATED
  -> CORE_STARTING
  -> CORE_READY
  -> OPTIONAL_STARTING
  -> HANDOFF_PREPARED
  -> HANDOFF_COMPLETE
  -> QUIESCENT (or IDLE if configured)

Failure rails:
  * REQUIRED_SERVICE_FAIL -> SAFE_MODE_PENDING -> SAFE_MODE_ACTIVE
  * DEADLINE_EXCEEDED     -> SAFE_MODE_PENDING -> SAFE_MODE_ACTIVE
  * HANDOFF_FAIL          -> DEGRADED_BOOT (profile policy decides)
```

State transition events should be emitted as typed records (not only logs), consumable by telemetry and fault services.

---

## 6) Data Contract Sketches (C-level)

## 6.1 Boot context (normalized input)

```c
typedef struct {
    uint32_t boot_reason;
    uint32_t reset_reason;
    uint32_t arch_id;
    uint32_t platform_id;
    uint32_t board_id;
    uint32_t personality_id;
    uint64_t cap_mask;
    uint64_t hw_feature_mask;
    bool safe_mode_requested;
    bool diagnostics_requested;
} init_boot_context_t;
```

## 6.2 Manifest service descriptor

```c
typedef struct {
    init_service_id_t id;
    const char *name;
    init_service_policy_t policy;
    uint8_t retry_limit;
    uint32_t start_deadline_ms;
    uint32_t ready_deadline_ms;
    const init_service_id_t *deps;
    uint8_t dep_count;
    uint64_t profile_mask;
    uint64_t board_mask;
    uint64_t personality_mask;
    uint64_t required_caps;
    int (*probe_fn)(const init_boot_context_t *ctx);
    int (*launch_fn)(const init_launch_request_t *req, init_launch_result_t *out);
} init_service_desc_t;
```

## 6.3 Handoff packet

```c
typedef struct {
    uint32_t boot_session_id;
    uint32_t final_boot_phase;
    uint32_t required_started;
    uint32_t required_failed;
    uint32_t optional_started;
    uint32_t optional_failed;
    uint32_t failure_class;
    uint32_t safe_mode_reason;
} init_handoff_summary_t;
```

---

## 7) Profile Behavior Matrix

| Profile | Manifest Style | Dependency Model | Retry Policy | Safe-mode Policy | Post-Boot Behavior |
|---|---|---|---|---|---|
| Tiny | compile-time static | strict static order | 0..1 attempts | fail-fast required only | quiescent/exit or idle stub |
| Embedded Rich | static + board capability filter | bounded dependency checks | bounded retries | degraded boot allowed for optional classes | handoff to servicemgr |
| RT/Safety | static deterministic | pre-validated DAG, deadline-aware | minimal retries, strict deadline | immediate safe-mode transition | handoff + watchdog registration |
| Mobile/Desktop | richer manifests | staged groups (core/late/optional) | bounded retries/backoff hint | diagnostics + recovery mode hooks | handoff + telemetry export |
| Cloud/Appliance (future) | profile package manifest | dependency+capability constraints | stronger restart strategy in supervisor | service-class based degradation | handoff to servicemgr/faultmgr |

---

## 8) Implementation Plan (Phased)

## Phase 1 — Harden Minimal Real Bootstrap (now)

**Goal:** make init practically useful without overreach.

Deliverables:
1. Boot context contract reader (`init_boot_context.c`) from kernel-passed block.
2. Real launch adapter for at least:
   - `namesvc`,
   - `servicemgr`.
3. Explicit boot phase transitions published to runtime channel.
4. Clear quiescent/idle behavior per profile.

Exit criteria:
- init can reliably bring up `namesvc` and `servicemgr` with deterministic order.

## Phase 2 — Manifest & Graph Validation

Deliverables:
1. Manifest schema extension with per-service deadlines.
2. Graph validator (cycle detection + deterministic ordering guarantees).
3. Required/optional classification reflected in machine-readable boot summary.
4. Basic offline manifest lint tool (`tools/` integration).

Exit criteria:
- invalid graphs fail before launch phase with explicit failure class.

## Phase 3 — Critical Boot Health Gate

Deliverables:
1. Required-service readiness/heartbeat gate.
2. Failure taxonomy (`INIT_FAIL_DEP`, `INIT_FAIL_TIMEOUT`, `INIT_FAIL_LAUNCH`, etc.).
3. Safe-mode reason encoding + handoff summary contract.
4. Fault manager integration hook for failure ingestion.

Exit criteria:
- boot failures are classifiable, observable, and acted upon by policy services.

## Phase 4 — Profile Adapters

Deliverables:
1. `tiny`, `rt`, `embedded_rich`, `mobile`, `desktop` adapters.
2. Per-profile mandatory service classes and deadline policies.
3. Build-system profile mapping cleanup (avoid macro sprawl).

Exit criteria:
- one init core supports multiple deployment classes without forked implementations.

## Phase 5 — Supervisor Handoff Protocol

Deliverables:
1. Typed IPC handoff contract to `servicemgr` and optional `faultmgr`.
2. Ownership transfer semantics documented (who monitors what, from when).
3. Boot-finalization acknowledgment path.

Exit criteria:
- long-term lifecycle authority is verifiably outside init.

---

## 9) Recommended Initial Service Sets by Profile

## Tiny
- Required: minimal payload service(s), optional `namesvc` only if IPC discovery is needed.
- Exclude by default: `process_manager`, `vm_manager`, heavy telemetry.

## Embedded Rich
- Required: `namesvc`, `servicemgr`.
- Optional/conditional: `faultmgr`, `telemetrymgr`, `process_manager`, `vm_manager` (if MMU).

## RT/Safety
- Required: `namesvc` (if used), watchdog bridge, safety policy endpoint.
- Optional strictly bounded: telemetry export.

## Mobile/Desktop
- Required: `namesvc`, `servicemgr`, `process_manager`.
- Optional staged: `vm_manager`, `faultmgr`, `telemetrymgr`, UI bootstrap services.

---

## 10) Risks and Guardrails

## Risks
- **Scope creep:** init absorbs supervision/fault policy.
- **Macro explosion:** profile behavior implemented via scattered `#ifdef` blocks.
- **Non-deterministic boot:** unbounded retries / late implicit dependency resolution.
- **Observability debt:** only textual logs, no structured boot records.

## Guardrails
- Keep init API narrow and phase-oriented.
- Move policy to profile adapters and dedicated managers.
- Enforce bounded operations and explicit deadlines.
- Require typed boot-state records and handoff summaries.

---

## 11) Definition of Done (for “proper init”) 

`core/services/init` is considered properly defined when all are true:

1. **Bootstrap-only authority:** init owns early boot, not lifelong supervision.
2. **Profile-aware behavior:** one engine, adapter policy per profile.
3. **Manifest-driven deterministic startup:** dependency/order/failure semantics explicit.
4. **Critical health gating:** required services are enforced with deadlines.
5. **Typed handoff:** `servicemgr`/`faultmgr` receives structured boot summary.
6. **Production observability:** boot phases and failure classes are machine-readable.

---

## 12) Immediate Next Actions (Repository Practical)

1. Align `core/services/core/init/README.md` language to “bootstrap orchestrator + handoff,” removing broad “root supervisor forever” phrasing.
2. Add `init_boot_context` kernel handoff contract header under `include/bharat/interface/uapi/init/`.
3. Replace `stub_start` for `namesvc` and `servicemgr` with real launch callbacks (even if minimal platform shim).
4. Implement phase publication and structured boot summary object.
5. Add `docs/architecture/core/services/init-handoff-protocol.md` to formalize transfer semantics.

This sequence preserves current progress while steering implementation toward a profile-native, non-monolithic Bharat-OS init.
