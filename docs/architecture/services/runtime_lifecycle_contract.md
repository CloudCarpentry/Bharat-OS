---
title: Service Runtime Lifecycle Contract
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
# Service Runtime Lifecycle Contract

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: 🚧 Pending core/kernel/service behavior merge
- **Validated**: ❌ Pending stress/fault-injection tests


## Overview

Bharat-OS enforces a strict architectural boundary: the kernel is mechanism only, while user-space services handle policy and orchestration. This document formalizes the runtime behavior of the core OS services responsible for lifecycle management: `servicemgr`, `process_manager`, and `vm_manager`.

All interaction with these services uses typed IPC message contracts defined in the `interface/uapi/` and `interface/idl/` directories, conforming to the Bharat-OS uRPC/IPC mechanisms.

## 1. servicemgr (Service Supervisor)

The `servicemgr` acts as the root supervisor and registry for all core system services.

### Core Responsibilities
- **Service Registry:** Maintains a table of running system services and their capabilities.
- **Lifecycle Management:** Starts and stops services via IPC boundaries.
- **Health Monitoring:** Enforces a periodic heartbeat. If a service misses its heartbeat window (`HEARTBEAT_TIMEOUT`), the service is transitioned to a `CRASHED` state.
- **Bounded Restart and Backoff:** Automatically restarts crashed services. If a service exceeds the `MAX_RESTART_COUNT`, it is placed into a `BACKOFF` state to prevent crash loops.

### State Machine
`UNKNOWN -> UNREGISTERED -> STOPPED -> STARTING -> RUNNING -> CRASHED -> BACKOFF`

### UAPI Interface
Defined in `include/bharat/interface/uapi/servicemgr/contract.h`:
- `SM_OP_REGISTER`: Registers a new service.
- `SM_OP_START`: Transitions service to `STARTING`.
- `SM_OP_STOP`: Transitions service to `STOPPED`.
- `SM_OP_QUERY`: Returns the current lifecycle state and restart count.
- `SM_OP_HEARTBEAT`: Service ping indicating health; moves state from `STARTING` to `RUNNING`.

---

## 2. process_manager (Process Orchestrator)

The `process_manager` handles high-level process lifecycle orchestration, policies, namespace setup, and process tree hierarchies.

### Core Responsibilities
- **Spawn Orchestration:** Creates new processes, handles capability seeding, and binds personalities.
- **State Tracking:** Maintains the active state of processes.
- **Reaping and Teardown:** Cleans up processes that have terminated or failed.

### State Machine
`UNKNOWN -> CREATED -> READY -> RUNNING -> STOPPING -> EXITED -> FAILED`

### UAPI Interface
Defined in `include/bharat/interface/uapi/process_manager/contract.h`:
- `PM_OP_CREATE`: Allocates process metadata and transitions to `CREATED`.
- `PM_OP_START`: Triggers execution, transitioning to `RUNNING`.
- `PM_OP_STOP`: Requests termination, transitioning to `STOPPING`.
- `PM_OP_QUERY`: Returns the current process state.

---

## 3. vm_manager (Virtual Memory Manager)

The `vm_manager` manages address space lifecycles, virtual regions, and memory mapping policies. It acts as the user-space orchestrator for the kernel's memory mechanisms.

### Core Responsibilities
- **Region Management:** Creates and tracks virtual memory regions for address spaces.
- **Page Fault Handling:** Responds to kernel page fault notifications (`VM_OP_FAULT`).
- **Memory Protection:** Manages memory protection flags (Read/Write/Execute).

### State Machine
`UNKNOWN -> DECLARED -> VALIDATED -> PROGRAMMED -> ACTIVE -> REVOKED`

### UAPI Interface
Defined in `include/bharat/interface/uapi/vm_manager/contract.h`:
- `VM_OP_MAP`: Declares a new memory region mapping (`DECLARED`).
- `VM_OP_UNMAP`: Revokes a memory region (`REVOKED`).
- `VM_OP_PROTECT`: Modifies memory protection flags (`VALIDATED`).
- `VM_OP_QUERY`: Returns region state.
- `VM_OP_FAULT`: Invoked by the kernel to request memory allocation/resolution on a fault (`ACTIVE`).

---

## Conclusion

By adhering to this message-based architecture, Bharat-OS ensures that core OS policies are enforced outside of the kernel while maintaining high resilience through strict supervisor hierarchies.
