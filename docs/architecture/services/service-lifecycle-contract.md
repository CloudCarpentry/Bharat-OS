---
title: Service Lifecycle Contract
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
# Service Lifecycle Contract

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: 🚧 Pending core/kernel/service behavior merge
- **Validated**: ❌ Pending stress/fault-injection tests


This document defines the generic contract for system and user-space service lifecycles in Bharat-OS. It serves as the definitive reference for how services transition between states, the boundaries of ownership during those transitions, and policies for recovery, failure, and readiness.

## 1. Goal & Scope

The goal of this contract is to standardize service management across the operating system without tying it to a single implementation subsystem (e.g., `init` or `servicemgr`). It defines *what* happens to a service, independent of *how* a specific manager implements it.

This contract applies to:
* Core kernel services
* Initial user-space services (Tier A)
* Dynamic system services (Tier B/C)

## 2. Service States

A service in Bharat-OS must adhere to the following deterministic states:

| State | Description | Transitions To |
|-------|-------------|----------------|
| **`CREATED`** | The service definition exists (e.g., manifest parsed), but no resources have been allocated or processes started. | `STARTING`, `FAILED` |
| **`STARTING`** | The service process has been spawned, resources are allocated, but it has not yet signaled readiness or completed its initialization sequence. | `RUNNING`, `FAILED`, `STOPPING` |
| **`RUNNING`** | The service is executing normally and has signaled readiness. It is actively servicing requests or performing its designated function. | `DEGRADED`, `STOPPING`, `FAILED` |
| **`DEGRADED`** | The service is running but experiencing issues (e.g., partial resource loss, missing dependencies, internal errors). It is not fully healthy. | `RUNNING`, `STOPPING`, `FAILED` |
| **`STOPPING`** | The service is in the process of shutting down gracefully. It may be flushing state, closing IPC channels, or releasing resources. | `STOPPED`, `FAILED` |
| **`STOPPED`** | The service has terminated cleanly. All resources are freed. It is no longer executing. | `STARTING` (if restarted), `CREATED` |
| **`FAILED`** | The service terminated unexpectedly (e.g., crash, fatal error, watchdog timeout) or failed to start. | `RESTART_BACKOFF`, `STOPPED` |
| **`RESTART_BACKOFF`** | The service is temporarily held in a waiting state after a failure before a restart is attempted, according to its backoff policy. | `STARTING`, `STOPPED` |

## 3. Ownership Boundaries

Clear boundaries define who is responsible for managing a service during different phases of its lifecycle.

### 3.1. The Kernel
* **Responsibility:** Mechanism only.
* **Scope:** The kernel is responsible for creating and destroying the underlying execution context (e.g., tasks, threads), mapping memory, and enforcing capability-based isolation.
* **Prohibited:** The kernel must **not** manage service states, restart policies, or readiness checks.

### 3.2. Service Manager / Subsystem Manager (e.g., `init`, `servicemgr`)
* **Responsibility:** Policy and orchestration.
* **Scope:** The manager is responsible for parsing manifests, applying the state machine defined in Section 2, enforcing dependency ordering, monitoring liveness (e.g., via watchdogs), and applying restart/backoff policies.
* **Prohibited:** The manager must **not** bypass kernel isolation or execute service logic directly.

### 3.3. The Service Itself
* **Responsibility:** Execution and signaling.
* **Scope:** The service is responsible for performing its business logic, gracefully handling `STOPPING` requests, and correctly signaling its `RUNNING` (readiness) state to its manager via standard IPC mechanisms.

## 4. Contract Policies

### 4.1. Readiness vs. Liveness
* **Readiness:** A service is considered `RUNNING` only *after* it explicitly signals readiness to its manager. This signal indicates it has initialized its IPC endpoints, acquired necessary resources, and is ready to accept requests.
* **Liveness:** A service is considered alive as long as its core execution context exists and it responds to periodic liveness checks (if configured). A failure to respond transitions the service to `DEGRADED` or `FAILED`.

### 4.2. Dependency Ordering
* Managers must respect declared dependencies. A service cannot transition to `STARTING` until all its strictly required dependencies have reached the `RUNNING` state.
* If a dependency transitions to `FAILED` or `STOPPED`, the manager must determine the impact on dependent services (e.g., transition them to `DEGRADED` or initiate a restart, based on policy).

### 4.3. Crash Handling and Restarts
* When a service terminates unexpectedly, it transitions to `FAILED`.
* The manager consults the service's policy. If restarts are enabled, the service transitions to `RESTART_BACKOFF`. If restarts are disabled or a limit is reached, it transitions to `STOPPED`.

### 4.4. Backoff Policy
* Restarts must implement an exponential backoff or similar rate-limiting policy to prevent rapid crash loops from consuming excessive system resources.
* The `RESTART_BACKOFF` state ensures a deterministic delay between a crash and the next `STARTING` attempt.

### 4.5. Essential vs. Optional Services
* **Essential:** Failure of an essential service (e.g., `namesvc`, `vfs` in some profiles) that exceeds its restart limits must trigger a system-wide recovery action or kernel panic.
* **Optional:** Failure of an optional service must be isolated. The service remains `STOPPED` or `FAILED` without impacting the overall system stability.

### 4.6. Observability Expectations
* All state transitions must be auditable. The manager is responsible for logging transitions (e.g., `STARTING` -> `RUNNING`, `RUNNING` -> `FAILED`) to a centralized telemetry or console service.

## 5. Future Tie-ins
As Bharat-OS evolves, this contract will integrate with:
* **Advanced Process Manager:** A dedicated process manager will take over Tier C service orchestration from the Tier A `init` system, adhering strictly to these states.
* **Supervision Trees:** Complex subsystems may implement internal supervision trees, where a parent service acts as a localized manager for child services, using this same state contract.
