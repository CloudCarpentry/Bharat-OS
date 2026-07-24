---
title: Bharat-OS Init Service Architecture (`core/services/init`)
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - services
see_also:
  - README.md
---
# Bharat-OS Init Service Architecture (`core/services/init`)

## 1. Overview and Core Philosophy

Bharat-OS is transitioning toward a service-oriented control plane where the kernel retains strictly deterministic mechanisms and services own policy and orchestration. In this architecture, `core/services/init` (located at `core/services/core/init`) is the **first policy orchestrator**. It is not a monolithic, permanently running god-process (like a traditional Linux `init` or Windows Service Control Manager).

Its primary jobs are:
1. **Establish the minimal runtime contract**: Handshake with the kernel, discover boot profiles/capabilities, and launch mandatory core services.
2. **Build the initial service graph**: Start foundational services like `namesvc`, `servicemgr`, and `faultmgr` based on the system profile.
3. **Get out of the way**: Hand over long-term supervision to dedicated supervisors (e.g., `servicemgr`) and domain-specific policies (e.g., `power_mode`, `telemetrymgr`) and either exit or remain as a tiny quiescent state holder depending on the profile.

This document describes the current baseline (stub) state and the fully developed future architecture for the multi-layered, profile-driven init service.

---

## 2. Current State vs. Future State

### Current State (Stub/Transition)
Currently, `core/services/core/init` is stubbed.
- The kernel uses `kernel_start_init_service()` (in `core/kernel/src/init/init_bootstrap.c`) to create a `sysmgr` process and thread.
- Full user-space ELF loading and true capability bootstrapping is deferred. A degrading boot mode is reported.
- `core/services/core/init/init_main.c` exists as a stub that initializes the runtime (`bharat_runtime_init()`), attempts to acquire a root capability (`bharat_runtime_get_bootstrap_cap()`), resolves a profile context (`init_profile_get_context()`), and runs a minimal status loop that suspends itself when done.

### Future State
The complete `core/services/init` will follow a layered approach heavily dependent on hardware profiles and manifest files, ensuring that from tiny MCUs to rich desktop systems, the boot orchestration scales gracefully.

---

## 3. Architecture Layers

The service architecture is divided into three layers to prevent `#ifdef` tangles and ensure cleanly separated responsibilities.

### Layer A: Universal Init Core
The engine that runs on every deployment, kept intentionally small.
- **Boot Context Intake**: Receives the initial kernel state, boot mode, and capabilities.
- **Profile Selection**: Uses the boot context to determine the current `bharat_init_profile_t`.
- **Manifest Loading**: Parses the static/compiled-in or dynamically provided `init_service_desc_t` manifest.
- **Dependency Ordering**: Resolves simple DAG or linear service dependencies.
- **Minimal Launch API**: Executes service binaries or spawns predefined threads.
- **Failure Classification**: Catches critical early failures (e.g., missing mandatory dependencies) and triggers `safe_mode` or system-halt.
- **Boot State Publication**: Signals phase transitions to the rest of the system.

### Layer B: Profile Behavior Adapters
Different profiles dictate how the core engine executes.
- `init_profile_tiny.c`: Minimal DAG, static capability resolution, minimal to no dynamic discovery.
- `init_profile_rt.c`: Deterministic static graph, strict deadlines, immediate fail-fast safe mode, early watchdog registration.
- `init_profile_embedded_rich.c`: Lightweight capability discovery, spawns `namesvc`, `servicemgr`, `faultmgr`.
- `init_profile_mobile.c` / `init_profile_desktop.c`: Richer manifests, staggered boot phases, diagnostic logging, and background services.
- `init_profile_cloud.c`: Strong emphasis on early networking/telemetry and service recovery.

### Layer C: Handoff Targets
Once `core/services/init` completes its bootstrap orchestration, responsibility is transitioned to domain experts:
- **`servicemgr`**: Service lifecycle, restart backoffs, dependency monitoring.
- **`faultmgr`**: Fault domain isolation and system-wide recovery policies.
- **`namesvc`**: General capability/namespace registration.
- **`telemetrymgr`**: Exporting the boot health logs.
- **`power_mode`**: Dynamic power state enforcement.

---

## 4. Boot State Machine and Manifests

### 4.1 State Machine Phases
The `init` bootstrap execution flows through defined, publishable phases:
1. `PHASE_EARLY_CONTEXT`: Kernel hands off root caps, `init` validates hardware profile and boot reason (e.g., normal vs. diagnostic OS upgrade).
2. `PHASE_MANIFEST_LOAD`: Profile adapter resolves the `g_init_manifest`. Capability preconditions (MMU, Networking) are mapped.
3. `PHASE_CORE_SERVICES`: Mandatory base services (`namesvc`, `servicemgr`, `faultmgr`) are launched and verified.
4. `PHASE_HEALTH_GATE`: Critical timeout and heartbeat checks are evaluated. If a critical service is missing, the state machine triggers a downgrade or OS restart diagnostic mode.
5. `PHASE_HANDOFF`: `servicemgr` takes over. `init` transitions to a `QUIESCENT` state or exits.

### 4.2 Manifest Structure (`init_manifest.h`)

Services are driven by a struct contract, keeping `init` logic uniform:
```c
typedef struct {
    init_service_id_t id;
    const char *name;
    int (*start_fn)(void *ctx);      // Or path for binary exec
    int (*probe_fn)(void *ctx);      // Capability/platform precheck
    const init_service_id_t *deps;
    uint8_t dep_count;
    uint8_t retry_limit;             // Only for critical early retries
    init_service_policy_t policy;    // REQUIRED vs OPTIONAL
    bharat_init_profile_mask_t profile_mask;
    bharat_init_cap_mask_t required_caps;
} init_service_desc_t;
```

---

## 5. Behavior Matrix by Profile

| Feature \ Profile | Tiny / MCU | RT / Safety | Embedded Rich | Mobile / Desktop / Cloud |
| :--- | :--- | :--- | :--- | :--- |
| **Manifest** | Static, compiled-in | Static, strict bounds | Static + Light discovery | Rich, dynamic files/DB |
| **Supervisor** | None/Self-managing | Pre-allocated static bounds | `servicemgr` | `servicemgr` |
| **Timeouts** | N/A or minimal | Strict deadline / fail-fast | Permissive | Highly staged / Deferred |
| **Handoff** | Idles / tiny state holder | Watchdog / Faultmgr handoff | Handoff to `servicemgr` | Handoff to `servicemgr` |
| **Safe Mode** | Halt / Reboot loop | Deterministic fallback profile | Basic diagnostic prompt | Interactive recovery UI |
| **Diagnostics** | Serial log only | Serial + Telemetry buffer | Telemetry + Storage | Rich diagnostic/update UI |

---

## 6. Integrations: Self-Test, Diagnostics, and OS Upgrade

- **Kernel Self-Test Integration**: The boot context passed to `init` will include results from the kernel self-test. If the self-test reported degraded hardware, `init` enforces a filtered `diagnostics_mode` profile, refusing to launch heavy payloads and launching only diagnostic/update services.
- **Restarts and OS Upgrades**: The kernel passes reboot reason flags to `init`. If booting from a failed upgrade (`FLAG_REVERT`) or an explicit diagnostic request (`FLAG_DIAG`), `init` selects an alternate, minimal manifest, ignoring optional desktop/cloud services.

## 7. Responsibilities Recap

**What `init` Does (Good Fit):**
- Boot phase state machine transitions.
- Parsing static/dynamic manifests.
- Startup ordering and enforcing dependency DAG.
- Health gating the critical boot window (timeouts).
- Seeding initial capabilities to `namesvc`/`servicemgr`.
- Handoff and graceful exit/suspension.

**What `init` Does NOT Do (Bad Fit):**
- Running indefinitely as a god-process service supervisor (use `servicemgr`).
- Power management or VM management (use dedicated managers).
- Device enumeration or UI policy.
- Hardcoded spaghetti logic for board-specific initializations (use `probe_fn` handlers or drivers).
