---
title: Process & Thread Management Architecture - Future Implementation Plan
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# Process & Thread Management Architecture - Future Implementation Plan

**Version:** v2.0 (Proposed - True Multikernel)
**Scope:** Kernel + Personality + Services
**Status:** Planned Actions

---

## Overview

Based on the [Process & Thread Management Architecture](process_thread.md), the Bharat-OS kernel's execution models will be refactored into a clear 3-layer architecture:
1. Kernel process model: **distributed, message-driven, no-global-state architecture** (`bh_process`, `bh_thread`)
2. Personality runtime layer: translate foreign semantics (Linux, Windows, Android, macOS)
3. User-space compatibility subsystems (Process Manager)

This document outlines the concrete, phased implementation steps to achieve this architecture.

---

## Phase 1: Core Kernel Refactoring (Removing Global State)

The immediate goal is to separate process/thread lifecycle management from the scheduler logic and **remove all global state**.

### Step 1.1: Introduce Per-Core State Structures
*   Create `core/kernel/include/proc/core_local.h` defining `struct core_local_state`.
*   Move global arrays like `g_threads` and `g_processes` into `core_local_state` as `local_threads` and `local_processes`.
*   Replace `g_urpc_states` with a `urpc_ring_t` implementation per core.

### Step 1.2: Create the `proc` Subsystem Scaffold
*   Create new directories: `core/kernel/include/proc/` and `core/kernel/src/proc/`.
*   Create headers: `core/kernel/include/proc/process.h` and `core/kernel/include/proc/thread.h`.
*   Create source files: `core/kernel/src/proc/process.c` and `core/kernel/src/proc/thread.c`.
*   Update `core/kernel/src/CMakeLists.txt` to include the new `proc` subdirectory.

### Step 1.3: Define the New Core Objects (Home Core Ownership)
*   In `process.h`, define the new `struct bh_process` with essential fields including `home_core` ownership tracking, a localized children list, and `proc_channel`.
*   In `thread.h`, define the new `struct bh_thread` with essential fields including `current_core` and `control_channel`.
*   *Note:* Ensure these definitions enforce that a process/thread belongs to one specific core.

### Step 1.4: Replace the Global Reaper
*   Remove the global `g_reap_lock`.
*   Implement a `local_reaper` queue within `core_local_state`.
*   Implement asynchronous ZOMBIE cleanup. When a thread on Core B exits and its parent is on Core A, Core B's local reaper sends a `THREAD_EXITED_EVENT` uRPC message to Core A.

### Step 1.5: Expand the Personality ABI Contract
*   Update `core/kernel/include/personality_ops.h` to expand `personality_ops_t`.
*   Add the new contract fields for distributed awareness: `create_process_model`, `create_thread_model`, `exit_process`, `clone_or_fork`, etc.
*   Update `core/kernel/src/personality/personality_default.c` with stub implementations.

---

## Phase 2: Async Revocation & Migration (The uRPC Layer)

Once local state is established, implement the cross-core communication primitives.

### Step 2.1: Implement `urpc_ring_t`
*   Replace legacy synchronous uRPC paths with lock-free ring buffers (`inbound_ring` / `outbound_ring`).
*   Implement backpressure and drop/retry policies for inter-core messages.

### Step 2.2: Async Capability Revocation
*   Refactor the capability system to use a 2-phase async revocation flow (REVOKE_REQ -> ACK) via uRPC instead of global polling/locking.

### Step 2.3: Async TLB Shootdowns
*   Replace global TLB locks with uRPC messages (`TLB_INVALIDATE(aspace, addr)`).

### Step 2.4: Implement Thread Migration
*   Implement the `MIGRATE_THREAD` uRPC flow to safely move a thread from Core A's runqueue to Core B's runqueue without shared memory mutation.

---

## Phase 3: User-Space Process Manager

With the distributed kernel core established, focus shifts to the user-space orchestrator.

### Step 3.1: Define the Process Manager BIDL Contract
*   Create a BIDL definition for the `process_manager` service defining operations for: `spawn`, `exec`, `kill`, `wait`, `reap`.

### Step 3.2: Implement the Process Manager Scaffold
*   Expand `core/services/process_manager/main.c` from its current TODO state.
*   Implement the IPC dispatch loop handling the new BIDL contract.
*   Ensure the Process Manager understands core targeting and issues capability grants to specific core-local endpoints.

---

## Phase 4: Linux Personality Runtime & Load Balancing

Future phases will build upon this foundation.

*   **Linux Personality Base:** Implement the `linux_personality_ops` bridging the expanded contract to POSIX semantics. Implement `fork()` using the neutral `process_create_native()` and explicit memory cloning, targeted via the Process Manager to a specific core.
*   **Android/Windows/macOS Personalities:** Develop additional personality runtimes utilizing the same distributed primitives.
*   **Cross-Core Load Balancing:** Implement an AI Governor or scheduling service that reads telemetry from local cores and issues `MIGRATE_THREAD` requests to balance load.