# Bharat-OS Current Code Status (March 2026)

This document summarizes **what is actually implemented in code today** versus what is still a scaffold/stub.
It is intended to complement architecture roadmaps with a code-backed snapshot.

## Scope and method

- Reviewed top-level build wiring (`CMakeLists.txt`, `services/CMakeLists.txt`, `subsys/CMakeLists.txt`).
- Reviewed service entry points (`services/*/main.c`, `services/*/src/main.c`).
- Reviewed non-trivial networking control/data plane modules and selected subsystem modules.

> Note: "Implemented" here means concrete logic exists in tree (not necessarily production-complete runtime behavior on all targets).

---

## 1) Build composition status

### Kernel + libraries
- Kernel, lib, drivers, subsystems, and services are all part of default top-level CMake build.
- Host tests are optional through `BHARAT_BUILD_HOST_TESTS`.

### Service composition
- Always built: `process_manager`, `vm_manager`, `file_system`, `drivers`, `crypto`, `console`, `boot_displayd`, legacy `net`.
- Default-on option groups:
  - `BHARAT_BUILD_USER_SERVICES_STUBS=ON`: `init`, `namesvc`, `servicemgr`.
  - `BHARAT_BUILD_CORE_SERVICES=ON`: `coremgr`, `memmgr`, `schedmgr`, `devmgr`, `accelmgr`, `storagemgr`, `faultmgr`, `telemetrymgr`.
  - `BHARAT_BUILD_NETWORK_STUBS=ON`: `netmgr`, `netstack`, `netfast`.

---

## 2) Service implementation matrix

| Service | Current code status | Evidence highlights |
| --- | --- | --- |
| `init` | Stub supervisor loop | Runtime bootstrap logs and TODO startup graph actions. |
| `namesvc` | Stub registry loop | Runtime initialization and TODO endpoint/lookup handling. |
| `servicemgr` | Stub | Prints init message and exits event loop placeholder. |
| `coremgr` | Stub | Topology/control responsibilities documented, logic TODO. |
| `memmgr` | Stub | Placeholder for page-fault/policy loop. |
| `schedmgr` | Stub | Placeholder policy service loop only. |
| `devmgr` | Stub | Placeholder for enumeration/hotplug. |
| `accelmgr` | Stub | Placeholder accelerator orchestration loop. |
| `storagemgr` | Stub | Placeholder storage policy loop. |
| `faultmgr` | Stub | Placeholder crash containment loop. |
| `telemetrymgr` | Stub | Placeholder metrics aggregation loop. |
| `process_manager` | Stub | TODO-only main. |
| `vm_manager` | Stub | TODO-only main. |
| `drivers` | Stub | TODO-only main. |
| `console` | Stub | Infinite loop with TODO URPC routing comments. |
| `boot_displayd` | Partial demo implementation | Includes framebuffer rectangle drawing helper and mocked early UI flow. |
| `file_system` | Partial scaffold | Calls `vfs_init()` and contains TODO mount/URPC flow. Roadmap includes FAT/littlefs support, persistent storage for IoT/edge, and OTA recovery. |
| `crypto` | Partial implementation | Initializes DRBG/keystore and validates/dispatches protocol requests via service modules; IPC transport path is currently stubbed. |
| `net` (legacy monolith) | Transitional + smoke-test logic | Control/data plane init and loopback smoke-test remain for compatibility. |
| `netmgr` | Partial implementation | Initializes interface/address/route/neighbor/driver-health state and handles multiple IPC opcodes with capability checks. |
| `netstack` | Partial implementation | Socket table + virtio adapter init plus protocol modules (IPv4, ARP, ICMP, UDP, loopback, Ethernet helpers). |
| `netfast` | Stub | Placeholder fast-path main. |

---

## 3) Networking implementation details (current)

## `services/netmgr` (control plane)
Implemented modules include:
- Interface table lifecycle (create/delete/get/admin-state).
- Address table add/remove/get.
- Route table add/remove/lookup (best prefix + metric tie-break).
- Neighbor cache add/remove/flush/lookup.
- Driver health registry/report/restart-intent bookkeeping.
- IPC opcode dispatcher that maps request types to those modules and fills response structures.

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
- `driver_virtio_adapter` is an adapter stub path for future integration depth.

---

## 4) Subsystem layer snapshot

- `subsys_manager` static library includes Linux/Windows compatibility shims, Android personality modules, automotive module, and SKB/test-runner wiring.
- Optional `ai_governor` demo executable is buildable and linked against scheduler-related kernel sources plus URPC/POSIX stub libraries.
- `subsys/network` defines the contract/types/uAPI surface for network manager and stack integration.

---

## 5) Practical takeaway for contributors

- Treat most non-network service daemons as **API/lifecycle scaffolding** at this stage.
- For concrete feature work, current depth is strongest in:
  1. networking split (`netmgr`, `netstack`),
  2. crypto service protocol/dispatch internals,
  3. subsystem contract surfaces and build/profile plumbing.
- If you are documenting capabilities externally, avoid calling all services "implemented"; classify many as "compiled stubs with defined responsibilities".
