---
title: Bharat-OS Current Code Status (March 2026)
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - dev
see_also:
  - README.md
---
# Bharat-OS Current Code Status (March 2026)

This document summarizes **what is actually implemented in code today** versus what is scaffolded.
It complements architecture roadmaps by grounding status in repository evidence.

## Maturity taxonomy (canonical)

Use these labels consistently across status docs and roadmap updates:

- **Scaffold**: Buildable skeleton/placeholders/TODO loops.
- **Partial**: Concrete logic exists in important paths, but end-to-end behavior is incomplete.
- **Baseline**: Core path works for integration/developer workflows.
- **Production**: Hardened reliability/security/observability and proven operationally.

> Architecture docs can remain forward-looking, but this file is the code-backed truth snapshot.

## Scope and method

- Reviewed build wiring (`CMakeLists.txt`, `core/services/CMakeLists.txt`, `subsys/CMakeLists.txt`).
- Reviewed service entry points (`core/services/*/main.c`, `core/services/*/src/main.c`).
- Reviewed non-trivial networking control/data-plane modules and selected subsystem modules.

---

## 1) Build composition status

### Kernel + libraries
- Kernel, lib, drivers, subsystems, and services are all part of the default top-level CMake build.
- Host tests are optional through `BHARAT_BUILD_HOST_TESTS`.

### Service composition

- Always built: `process_manager`, `vm_manager`, `file_system`, `drivers`, `crypto`, `console`, `boot_displayd`. legacy `net`.
- Experimental option groups (`BHARAT_BUILD_EXPERIMENTAL_SERVICES=OFF` by default):
  - `BHARAT_BUILD_USER_SERVICES_STUBS=ON`: `init`, `namesvc`, `servicemgr`.
  - `BHARAT_BUILD_CORE_SERVICES=ON`: `coremgr`, `memmgr`, `schedmgr`, `devmgr`, `accelmgr`, `storagemgr`, `faultmgr`, `telemetrymgr`.
  - Also guards: `netfast`, `sensor_hub`, `time_sync`, `can`, `power_mode`.
- Forward path network stack (`BHARAT_BUILD_NETWORK_STUBS=ON`): `netmgr`, `netstack` (default ON).
- Legacy/Transitional network stack (`BHARAT_ENABLE_SERVICE_NETWORK=ON`): `net` (default ON but deprecated).
-  Default-off option groups (enabled in experimental presets):
  - `BHARAT_BUILD_USER_SERVICES_STUBS=ON`: `init`, `namesvc`, `servicemgr`.
  - `BHARAT_BUILD_CORE_SERVICES=ON`: `coremgr`, `memmgr`, `schedmgr`, `devmgr`, `storagemgr`, `faultmgr`, `telemetrymgr`.
  - `BHARAT_BUILD_NETWORK_STUBS=ON`: `netmgr`, `netstack`, `netfast`.


## 2) Service implementation matrix (taxonomy-aligned)

| Service | Current maturity | Evidence highlights |
| --- | --- | --- |
| `init` | **Scaffold** | Bootstrap logs + TODO startup graph actions. |
| `namesvc` | **Partial** | Registry operations are implemented, but endpoint/capability flow is mocked and loop behavior is not production-hardened. |
| `servicemgr` | **Scaffold** | Init path + placeholder event loop. |
| `coremgr` | **Scaffold** | Responsibility skeleton only. |
| `memmgr` | **Scaffold** | Placeholder for page-fault/policy loop. |
| `schedmgr` | **Scaffold** | Placeholder policy service loop only. |
| `devmgr` | **Scaffold** | Placeholder for enumeration/hotplug. |
| `accelmgr` | **Scaffold** | Placeholder orchestration loop. |
| `storagemgr` | **Scaffold** | Placeholder storage policy loop. |
| `faultmgr` | **Scaffold** | Placeholder crash containment loop. |
| `telemetrymgr` | **Scaffold** | Placeholder metrics aggregation loop. |
| `process_manager` | **Scaffold** | TODO-only main path. |
| `vm_manager` | **Scaffold** | TODO-only main path. |
| `drivers` | **Scaffold** | TODO-only main path. (Registry contract is **Partial/D0 Baseline**). |
| `console` | **Scaffold** | Infinite loop + TODO URPC routing. |
| `boot_displayd` | **Partial** | Framebuffer rectangle helper + mocked early UI flow. |
| `file_system` | **Partial** | Calls `vfs_init()` and outlines mount/URPC flow; persistent backing FS path still pending. |
| `crypto` | **Partial** | DRBG/keystore + request validation/dispatch logic; IPC transport path is stubbed. |
| `net` (legacy monolith) | **Partial** | **Deprecated/Transitional** control/data plane and smoke-test compatibility path. |
| `netmgr` | **Baseline** | Interface/address/route/neighbor/driver-health modules, full IPC opcode dispatcher, capability wiring, health tracking and yield loop. Forward path. |
| `netstack` | **Partial** | Socket table + protocol modules (IPv4/ARP/ICMP/UDP/loopback/Ethernet) + virtio adapter init hook. Forward path. |
| `netfast` | **Scaffold** | Placeholder fast-path main. |

---

## 3) Core Subsystem implementation details

### Capability Validation Framework
**Status: Partial / Phase K3-S0 baseline**

A reusable, strict, fail-closed capability validation framework has been introduced to the kernel. This serves as the canonical validation path for object types, rights, scope (PID-based), and generation/stale-handle checks.

**Current progress:**
- Canonical `cap_validate_ex()` implementation.
- Internal helpers for granular validation (rights, type, scope, generation).
- Rich internal error mapping via `kstatus_t` (e.g., `K_ERR_CAP_STALE`, `K_ERR_CAP_REVOKED`).
- Comprehensive negative-path unit tests in `ktest_capability`.
- Initial contract documentation in `docs/architecture/kernel/capability-validation-contract.md`.

**Current limitations:**
- Not yet wired into every syscall boundary.
- Not yet enforced across all IPC/service entry points.
- Capability lifecycle revocation semantics are still partial.
- Security-domain scope model is intentionally minimal (PID-only) in this phase.

### Driver Registry Contract
**Status: Partial / D0 Baseline**

The driver registry and device binding contract has been hardened to provide a deterministic matching and lifecycle management path. This decouples hardware description from driver operation and tracks the active relationship.

**Current progress:**
- Established canonical `device_desc_t`, `driver_desc_t`, and `device_binding_t` descriptors.
- Deterministic match scoring: Highest score wins, then highest priority, then reject ambiguity.
- Formalized lifecycle state machine (REGISTERED, MATCHED, PROBED, STARTED, STOPPED, REMOVED, FAILED) enforced at the core level.
- Host-side verification tests for registration, matching, and lifecycle transitions.
- Preliminary documentation in `docs/architecture/drivers/driver-registry-contract.md`.

**Current limitations:**
- Driver service runtime remains a scaffold.
- Capability mediation for driver operations is documented but not yet wired.
- Most existing drivers are still skeletal and do not fully implement the new lifecycle hooks.

---

## 4) Networking implementation details

*Note: The legacy `net` monolithic service is deprecated and transitional. `netmgr` and `netstack` represent the canonical forward path.*

## `core/services/netmgr` (control plane)
Implemented modules include:
- Interface table lifecycle (create/delete/get/admin-state).
- Address table add/remove/get.
- Route table add/remove/lookup (best prefix + metric tie-break).
- Neighbor cache add/remove/flush/lookup.
- Driver health registry/report/restart-intent bookkeeping.
- IPC opcode dispatcher mapping request types into those modules.

Current limitations:
- Main event loop intentionally breaks immediately (daemon runtime not fully wired).
- Capability checks now use a strict fail-closed shim via `bharat_cap_validate()` (denies by default). The previous permissive bypass is obsolete, though the full capability lookup engine is still pending.
- Restart behavior records intent only; no process-manager integration yet.
- Full blocking IPC wait is waiting on scheduler integration; falls back to yielding loop for now.
- System registry bindings are still mocked until `namesvc` is fully operational.

## `core/services/netstack` (data plane)
Implemented modules include:
- Net buffer manipulation and checksum helpers.
- IPv4 RX/TX parsing + checksum validation + local/loopback/broadcast handling.
- UDP RX/TX with pseudo-header checksum and socket callback delivery.
- ICMP/ARP/Ethernet/loopback modules and socket table support.

Current limitations:
- Main loop is scaffolded and not yet driving timers/driver polling end-to-end.
- `driver_virtio_adapter` is an adapter stub path for deeper integration.

---

## 5) Gap summary (docs-to-code)

1. **Status precision gap**: several services are buildable but still lifecycle scaffolds.
2. **Enforcement gap**: capability checks in some manager paths were historically placeholder-permissive; however, the major `netmgr` bypass has been closed (fail-closed shim). Full capability routing is still needed. The Phase K3-S0 validation framework provides the tools to close this gap.
3. **Runtime wiring gap**: multiple daemons initialize state but do not yet run complete production event loops.
4. **Hardening gap**: observability, failure semantics, and profile-specific guarantees need deeper closure.

## 5.1) Security production gate (explicit blocker)

To avoid status inflation, this repository treats capability enforcement maturity as a hard release gate:

- **No production claim is valid while capability mediation remains Partial.**
- A manager path must not rely on default-allow, placeholder, or stub authorization behavior.
- Fail-closed behavior (like the current `netmgr` shim through `bharat_cap_validate()`) is required as an interim baseline, but does not by itself qualify as full production security.
- Promotion to **Production** requires strict, code-backed capability checks across manager and dispatch paths, plus validation evidence.

---

## 6) Contributor update rules

When updating this document:

1. Use only the four canonical maturity labels.
2. Prefer conservative labels when runtime behavior is incomplete.
3. Add evidence notes for any status promotion.
4. If architecture docs are more aspirational, retain them, but record the current reality here.


## 7) Component architecture drill-down docs

For diagram-based decomposition and roadmap mapping by domain, see:

- `docs/architecture/components/kernel-subcomponents-architecture.md`
- `docs/architecture/components/subsystem-subcomponents-architecture.md`
- `docs/architecture/components/services-subcomponents-architecture.md`
- `docs/architecture/components/drivers-subcomponents-architecture.md`

## 8) Memory hardening roadmap

For the current memory production hardening plan and profile/architecture acceptance matrix, see:

- `docs/architecture/memory/memory-roadmap-consolidated.md`

## 9) Service Deployment Profiles

Services in Bharat-OS are not universally mandatory. They are deployed via three primary profile tiers to accommodate hardware constraints:

*   **Tier A (Tiny profile):** `init`, `namesvc`, `devmgr`, `process_manager`, `vm_manager`.
    *Target:* Micro edge nodes, sensors, controller-class devices, low-storage builds.
*   **Tier B (Managed embedded profile):** Tier A + `servicemgr`, `faultmgr`, minimal telemetry export.
    *Target:* Serious embedded products needing restart, supervision, and fleet debugging.
*   **Tier C (Rich embedded/mobile/appliance profile):** Tier B + `storagemgr`, `telemetrymgr`, `console`, `boot_displayd`, optional `accelmgr`.
    *Target:* UI devices, media devices, connected appliances, richer edge systems.

*Note: Core system managers like `schedmgr`, `memmgr`, and `coremgr` are profile-optional and only engaged when policy must be strictly separated from kernel mechanisms.*
