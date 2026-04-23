# services/init

## Purpose
The first trusted user-space bootstrap service, acting strictly as a **bootstrap coordinator**. It launches the minimum control-plane graph, enforces startup order for early boot, applies health gates based on kernel boot status, and handles early boot failure policy (for example safe mode). It is designed to be profile-aware and deterministic.

**Crucially, `init` is NOT a permanent service supervisor, process tree owner, or a Linux-style systemd clone.** After early boot convergence and graph validation, `init` hands off long-lived lifecycle management, fault handling, restart policy, and dependency supervision to `servicemgr` (with `faultmgr` integration). It may then exit or become quiescent depending on profile.

## Control-plane split (authoritative model)

The Bharat-OS control plane is intentionally split into three layers:

1. **`init`**: trusted bootstrap only.
2. **`servicemgr`**: long-lived lifecycle authority.
3. **policy manager (`policymgr`, profile-dependent)**: runtime policy decisions based on profile + hardware + policy config.

This prevents policy-heavy orchestration from collapsing into a single permanent init daemon and keeps the kernel/services separation clean.

## Bootstrap lifecycle

### Stage 0: Kernel handoff (mechanism only)
The kernel performs mechanism-level bring-up and launches the first trusted user-space service.

### Stage 1: `init` bootstrap
`init` validates boot context and starts the minimum control-plane services:
- `capmgr`
- `devmgr`
- `process_manager`
- `vm_manager`
- `servicemgr`
- optional `policymgr` (profile-specific)

### Stage 2: `servicemgr` takeover
`servicemgr` becomes lifecycle owner:
- service start/stop/restart
- dependency satisfaction
- restart/backoff tracking
- watchdog + health contracts
- boot-phase transitions and degraded-mode handling

### Stage 3: policy-driven runtime
`policymgr` (when present) selects runtime behavior by profile/hardware policy:
- allowed service sets
- scheduler and power posture
- lazy/dynamic activation posture
- fault/degradation policy

## Profile guidance

- **Small RT/appliance/wearable:** static manifest, fixed graph, minimal `init`, compact `servicemgr`, optional compile-time policy.
- **Automotive/robotics/medical:** safety-first boot classes, strict supervision, watchdog/fault-domain integration, degraded-mode guarantees.
- **Desktop/server/cloud:** dynamic manifests, richer dependency graphs, stronger telemetry and runtime policy.

## Dependencies
- **May depend on:** `lib/runtime`, `lib/ipc`, `lib/cap`, standard C library headers.
- **Must not depend on:** other `services/*` (no service-to-service compile-time dependency), `subsys/*`, `ui/*`.
- **Must not use:** direct kernel-private headers or duplicate kernel self-test logic.

## Planned Public API
- Subsystem init hooks / normalized boot context contract (`init_boot_context.h`).
- Structured handoff protocol to `servicemgr`.
- Machine-readable boot status/failure reporting.

## Status
Evolving into the capability-aligned bootstrap coordinator phase.
