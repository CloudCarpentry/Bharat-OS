---
title: Bharat-OS `core/services/init` — Profile Implementation Task Plan
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - services
see_also:
  - README.md
---
# Bharat-OS `core/services/init` — Profile Implementation Task Plan

## 1. Purpose

This task plan translates the `init_architecture` and `init_bootstrap_contract` documents into an implementation backlog for multiple product profiles:

- Automobile
- TV
- Appliance (IoT/home devices)
- Watch (wearable)
- Mobile
- Desktop
- Cloud

The intent is to keep `init` as a **short-lived bootstrap coordinator**, while profile managers and `servicemgr` own long-term policy.

---

## 2. Common Baseline (Implement Once)

These tasks are mandatory before profile-specific behavior can work.

### T0 — Boot Context + Kernel Health Contract
- Define `init_boot_context_t` and `kboot_health_summary_t` headers shared by kernel + init.
- Pass reboot reason, safe mode request, profile hint, capability mask, and kernel self-test results.
- Reject invalid/unknown contract versions.

### T1 — Init State Machine Core
Implement deterministic state machine phases:
1. `CONTEXT_READY`
2. `PROFILE_SELECTED`
3. `MANIFEST_SELECTED`
4. `GRAPH_VALIDATED`
5. `CORE_STARTING`
6. `CORE_READY`
7. `OPTIONAL_STARTING`
8. `HANDOFF_PREPARED`
9. `HANDOFF_COMPLETE`
10. `QUIESCENT/EXIT`

### T2 — Manifest Engine + Boot Classes
- Add manifest schema with boot classes:
  - `BOOT_CLASS_CORE`
  - `BOOT_CLASS_INFRA`
  - `BOOT_CLASS_OPTIONAL`
  - `BOOT_CLASS_LATE`
  - `BOOT_CLASS_DIAGNOSTIC`
- Support profile mask + required capability mask filtering.
- Validate dependency graph (cycle detection + missing dependency errors).

### T3 — Event-Driven Readiness Tracking
- Track service registration/readiness events from `namesvc`.
- Enforce per-service deadlines and global phase deadlines.
- Remove unbounded retry loops.

### T4 — Failure Classification + Boot Outcomes
- Implement failure codes:
  - `INIT_FAIL_DEP`
  - `INIT_FAIL_TIMEOUT`
  - `INIT_FAIL_LAUNCH`
  - `INIT_FAIL_CAPABILITY`
  - `INIT_FAIL_PROFILE`
  - `INIT_FAIL_HEARTBEAT`
- Map failures to outcomes: normal, degraded, safe mode, diagnostic.

### T5 — Structured Handoff Protocol
- Emit `init_handoff_summary_t` + per-service status to:
  - `servicemgr`
  - `faultmgr`
- Require ACK before final `HANDOFF_COMPLETE`.

### T6 — Quiesce/Exit Policy
- Profile hook decides if init exits or remains quiescent state holder.
- Ensure no long-running supervision loop remains in init.

---

## 3. Profile-Specific Execution Models

## 3.1 Automobile Profile
### How it should work
- Safety-first deterministic startup.
- Fast bring-up of fault handling, telemetry, and critical vehicle-control support services.
- Optional infotainment stack starts only after core readiness.

### Implementation tasks
- Define `INIT_PROFILE_AUTOMOTIVE` adapter.
- Strict deadlines for CORE/INFRA; immediate safe-mode downgrade on miss.
- Dual domain startup classes:
  - `vehicle_core` (critical)
  - `infotainment` (optional/late)
- Add policy: boot allowed with infotainment failure, but not with control/fault path failure.
- Ensure early `faultmgr` availability before handoff.

## 3.2 TV Profile
### How it should work
- Fast time-to-home-screen.
- Core multimedia and input pipeline early, app ecosystem later.

### Implementation tasks
- Define `INIT_PROFILE_TV` adapter.
- Mark display/input compositor dependencies as `INFRA`.
- Defer app store, recommendation, analytics, and heavy services to `LATE`.
- Add degraded mode that still supports remote control + local playback.

## 3.3 Appliance Profile
### How it should work
- Tiny/embedded style predictable boot with low memory footprint.
- Device function first (e.g., thermostat, washer, sensor hub), UI/network optional by SKU.

### Implementation tasks
- Define `INIT_PROFILE_APPLIANCE` adapter.
- Use static manifest by default; avoid dynamic loader dependency where possible.
- No heavy retries; bounded one-shot startup.
- Optional cloud connectivity gated by capability and SKU flags.
- If required control loop service fails, enter safe mode immediately.

## 3.4 Watch (Wearable) Profile
### How it should work
- Battery-aware staged boot.
- Sensor/time/notifications early; richer background services deferred.

### Implementation tasks
- Define `INIT_PROFILE_WATCH` adapter.
- CORE includes timebase, sensor hub IPC, notification bridge, minimal UI shell.
- Health gate includes battery/thermal constraints from boot context where available.
- Defer sync, analytics, and non-critical companions to `LATE`.
- Quiesce quickly after handoff to minimize overhead.

## 3.5 Mobile Profile
### How it should work
- Staged boot with strict core readiness and rich deferred startup.
- Must survive partial failures and continue in degraded mode.

### Implementation tasks
- Define `INIT_PROFILE_MOBILE` adapter.
- Require namesvc/servicemgr/faultmgr + minimal display/input path before user-ready signal.
- Split boot into foreground critical vs background optional service groups.
- Add diagnostic branch for failed update/revert boot reason.
- Enable rich boot telemetry export via `telemetrymgr`.

## 3.6 Desktop Profile
### How it should work
- Similar to mobile but with larger optional graph and stronger diagnostics.
- Support recovery/repair sessions when core services fail.

### Implementation tasks
- Define `INIT_PROFILE_DESKTOP` adapter.
- Require session core (`display`, `input`, `identity/session broker`) before handoff complete.
- Launch diagnostics/recovery UI in safe/diagnostic mode.
- Keep non-essential developer tooling and background indexing in `LATE`.

## 3.7 Cloud Profile
### How it should work
- Infrastructure readiness over full stack readiness.
- Early networking, control-plane registration, observability.
- Rapid handoff to orchestrators.

### Implementation tasks
- Define `INIT_PROFILE_CLOUD` adapter.
- Make network/control-plane agent/identity trust services `CORE` or `INFRA`.
- Do not block boot on tenant/optional workloads.
- Always emit structured boot summary for external control-plane visibility.
- Aggressive degraded classification instead of local UI-centric recovery.

---

## 4. Cross-Profile Milestones

### M1 — API/Contract Foundation
Deliver: T0, T1 headers + state machine skeleton + unit tests for phase transitions.

### M2 — Manifest + Dependency Engine
Deliver: T2 + graph validator tests (cycle, missing dep, profile filter).

### M3 — Event/Deadline/Failure Core
Deliver: T3 + T4 + timeout simulation tests.

### M4 — Handoff + Quiesce
Deliver: T5 + T6 + integration tests with mocked `servicemgr`/`faultmgr` ACK.

### M5 — First Profile Set
Deliver adapters: Appliance, Watch, Mobile.

### M6 — Full Profile Coverage
Deliver adapters: Automotive, TV, Desktop, Cloud.

### M7 — Safe Mode + Diagnostics Hardening
Deliver reboot-reason policy + failed-upgrade diagnostic manifest handling.

---

## 5. Suggested Initial Backlog (Actionable Tickets)

1. Add `core/services/core/init/include/init_boot_context.h` with versioned context structs.
2. Add `core/services/core/init/include/init_manifest.h` with boot classes and descriptor schema.
3. Implement `init_state_machine.c` with explicit transition table.
4. Implement `init_manifest_engine.c` with profile/capability filtering.
5. Implement `init_dependency_validator.c` for DAG validation.
6. Implement `init_event_loop.c` for registration/readiness/deadline events.
7. Implement `init_failure_policy.c` mapping failures to boot outcomes.
8. Implement `init_handoff_protocol.c` with ACK timeout policy.
9. Add profile adapter files:
   - `init_profile_automotive.c`
   - `init_profile_tv.c`
   - `init_profile_appliance.c`
   - `init_profile_watch.c`
   - `init_profile_mobile.c`
   - `init_profile_desktop.c`
   - `init_profile_cloud.c`
10. Add test suites for contract parsing, phase transitions, and profile policy decisions.

---

## 6. Definition of Completion

Profile implementation is done when:
- Every profile boots through the common init state machine.
- CORE/INFRA services are enforced by policy and deadlines.
- Failure classification deterministically selects normal/degraded/safe/diagnostic.
- Handoff to `servicemgr`/`faultmgr` is structured and ACKed.
- Init exits or quiesces with no long-term supervision responsibilities.
