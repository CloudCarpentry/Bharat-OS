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

- Reviewed build wiring (`CMakeLists.txt`, `services/CMakeLists.txt`, `subsys/CMakeLists.txt`).
- Reviewed service entry points (`services/*/main.c`, `services/*/src/main.c`).
- Reviewed non-trivial networking control/data-plane modules and selected subsystem modules.

---

## 1) Build composition status

### Kernel + libraries
- Kernel, lib, drivers, subsystems, and services are all part of the default top-level CMake build.
- Host tests are optional through `BHARAT_BUILD_HOST_TESTS`.

### Service composition
- Always built: `process_manager`, `vm_manager`, `file_system`, `drivers`, `crypto`, `console`, `boot_displayd`, legacy `net`.
- Default-on option groups:
  - `BHARAT_BUILD_USER_SERVICES_STUBS=ON`: `init`, `namesvc`, `servicemgr`.
  - `BHARAT_BUILD_CORE_SERVICES=ON`: `coremgr`, `memmgr`, `schedmgr`, `devmgr`, `accelmgr`, `storagemgr`, `faultmgr`, `telemetrymgr`.
  - `BHARAT_BUILD_NETWORK_STUBS=ON`: `netmgr`, `netstack`, `netfast`.

---

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
| `drivers` | **Scaffold** | TODO-only main path. |
| `console` | **Scaffold** | Infinite loop + TODO URPC routing. |
| `boot_displayd` | **Partial** | Framebuffer rectangle helper + mocked early UI flow. |
| `file_system` | **Partial** | Calls `vfs_init()` and outlines mount/URPC flow; persistent backing FS path still pending. |
| `crypto` | **Partial** | DRBG/keystore + request validation/dispatch logic; IPC transport path is stubbed. |
| `net` (legacy monolith) | **Partial** | Transitional control/data plane and smoke-test compatibility path. |
| `netmgr` | **Partial** | Interface/address/route/neighbor/driver-health modules + IPC opcode dispatcher. |
| `netstack` | **Partial** | Socket table + protocol modules (IPv4/ARP/ICMP/UDP/loopback/Ethernet) + virtio adapter init hook. |
| `netfast` | **Scaffold** | Placeholder fast-path main. |

---

## 3) Networking implementation details

## `services/netmgr` (control plane)
Implemented modules include:
- Interface table lifecycle (create/delete/get/admin-state).
- Address table add/remove/get.
- Route table add/remove/lookup (best prefix + metric tie-break).
- Neighbor cache add/remove/flush/lookup.
- Driver health registry/report/restart-intent bookkeeping.
- IPC opcode dispatcher mapping request types into those modules.

Current limitations:
- Main event loop intentionally breaks immediately (daemon runtime not fully wired).
- Capability checks currently allow all requests (`netmgr_cap_check_rights` returns true).
- Restart behavior records intent only; no process-manager integration yet.

## `services/netstack` (data plane)
Implemented modules include:
- Net buffer manipulation and checksum helpers.
- IPv4 RX/TX parsing + checksum validation + local/loopback/broadcast handling.
- UDP RX/TX with pseudo-header checksum and socket callback delivery.
- ICMP/ARP/Ethernet/loopback modules and socket table support.

Current limitations:
- Main loop is scaffolded and not yet driving timers/driver polling end-to-end.
- `driver_virtio_adapter` is an adapter stub path for deeper integration.

---

## 4) Gap summary (docs-to-code)

1. **Status precision gap**: several services are buildable but still lifecycle scaffolds.
2. **Enforcement gap**: capability checks in some manager paths are placeholder-permissive.
3. **Runtime wiring gap**: multiple daemons initialize state but do not yet run complete production event loops.
4. **Hardening gap**: observability, failure semantics, and profile-specific guarantees need deeper closure.

---

## 5) Contributor update rules

When updating this document:

1. Use only the four canonical maturity labels.
2. Prefer conservative labels when runtime behavior is incomplete.
3. Add evidence notes for any status promotion.
4. If architecture docs are more aspirational, retain them, but record the current reality here.


## 6) Component architecture drill-down docs

For diagram-based decomposition and roadmap mapping by domain, see:

- `docs/architecture/components/kernel-subcomponents-architecture.md`
- `docs/architecture/components/subsystem-subcomponents-architecture.md`
- `docs/architecture/components/services-subcomponents-architecture.md`
- `docs/architecture/components/drivers-subcomponents-architecture.md`

## 7) Memory hardening roadmap

For the current memory production hardening plan and profile/architecture acceptance matrix, see:

- `docs/architecture/memory-production-grade-plan.md`
