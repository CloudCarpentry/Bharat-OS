# Process & Thread Management Architecture - Future Implementation Plan

**Scope:** Kernel + Personality + Services
**Status:** Planned Actions

---

## Overview

Based on the [Process & Thread Management Architecture](process_thread.md), the Bharat-OS kernel's execution models will be refactored into a clear 3-layer architecture:
1. Kernel process model: one neutral execution model (`kprocess`, `kthread`)
2. Personality runtime layer: translate foreign semantics (Linux, Windows, Android, macOS)
3. User-space compatibility subsystems (Process Manager)

This document outlines the concrete, phased implementation steps to achieve this architecture, starting with minimal "baby steps" to cleanly extract existing logic from `sched.c`.

---

## Phase 1: Core Kernel Refactoring (The "Baby Steps")

The immediate goal is to separate process/thread lifecycle management from the scheduler logic without breaking the existing system.

### Step 1.1: Create the `proc` Subsystem Scaffold
*   Create new directories: `kernel/include/proc/` and `kernel/src/proc/`.
*   Create headers: `kernel/include/proc/process.h` and `kernel/include/proc/thread.h`.
*   Create source files: `kernel/src/proc/process.c` and `kernel/src/proc/thread.c`.
*   Update `kernel/src/CMakeLists.txt` to include the new `proc` subdirectory.

### Step 1.2: Define the New Core Objects
*   In `process.h`, define the new `struct kprocess` with essential fields (pid, parent_pid, state, aspace, cspace, main_thread, personality, children list, waiters, exit_code, owner_core).
*   In `thread.h`, define the new `struct kthread` with essential fields (tid, process pointer, state, kernel_stack, context, priority, affinity, tls, signals, wait_channel, current_core, personality).
*   *Note:* Ensure these definitions initially map closely to existing structures in `sched.h` to minimize breakage, adding new fields (like TLS, signals, lifecycle states) as foundational stubs.

### Step 1.3: Extract Lifecycle Functions from `sched.c`
*   Move `process_create()` logic from `kernel/src/sched/sched.c` to `kernel/src/proc/process.c`. Rename/refactor it towards `process_create_native()`.
*   Move `thread_create()` logic from `kernel/src/sched/sched.c` to `kernel/src/proc/thread.c`. Rename/refactor it towards `thread_create_native()`.
*   Ensure `sched.c` only contains runqueue, pick, preempt, tick, and migration logic. It should call into the new `proc` API for object creation/teardown.

### Step 1.4: Expand the Personality ABI Contract
*   Update `kernel/include/personality_ops.h` to expand `personality_ops_t` from its current minimal 3 callbacks.
*   Add the new contract fields:
    *   `create_process_model`
    *   `create_thread_model`
    *   `exit_process`
    *   `exit_thread`
    *   `clone_or_fork`
    *   `prepare_exec`
    *   `deliver_signal_or_exception`
    *   `wait_event_translate`
    *   `map_scheduler_policy`
    *   `map_object_namespace`
*   Update `kernel/src/personality/personality_default.c` to implement stub versions of these new operations to ensure the kernel compiles.

### Step 1.5: Implement Basic Wait/Exit Stubs
*   Create `kernel/src/proc/exit.c` and `kernel/src/proc/wait.c`.
*   Implement basic stubs for `process_exit()`, `thread_exit()`, and `wait_object()`, handling the transition to `ZOMBIE` state and basic parent notification logic.

---

## Phase 2: User-Space Process Manager

Once the kernel core is cleanly separated, focus shifts to the user-space orchestrator.

### Step 2.1: Define the Process Manager BIDL Contract
*   Create a BIDL definition for the `process_manager` service defining operations for:
    *   `spawn`
    *   `exec`
    *   `kill`
    *   `wait`
    *   `reap`

### Step 2.2: Implement the Process Manager Scaffold
*   Expand `services/process_manager/main.c` from its current TODO state.
*   Implement the IPC dispatch loop handling the new BIDL contract.
*   Implement stub handlers that log requests and return standard errors, preparing for full implementation.

---

## Phase 3: Linux Personality Runtime (The MVP)

With the neutral kernel primitives and a process manager in place, implement the first concrete personality.

### Step 3.1: Linux Personality Base
*   Create a dedicated `personalities/linux/` directory if not already properly structured.
*   Implement the `linux_personality_ops` bridging the expanded `personality_ops_t` contract to POSIX semantics.

### Step 3.2: Implement `fork`/`clone` Semantics in User-Space
*   Implement the Linux `fork()` behavior entirely within the Linux personality runtime, utilizing the neutral `process_create_native()` and explicit memory cloning (COW) rather than a kernel-level `fork`.

### Step 3.3: POSIX Signals and Wait
*   Implement POSIX signal delivery by mapping the kernel's generic async event primitive.
*   Implement `waitpid` by mapping to the kernel's generic wait queues and the process manager's reaping logic.

---

## Phase 4: Advanced Integration & Multikernel

Future phases will build upon this foundation.

*   **Android/Windows/macOS Personalities:** Develop additional personality runtimes utilizing the same neutral kernel primitives.
*   **Thread Migration:** Implement the multikernel thread migration flow (MIGRATE_THREAD via uRPC) across cores.
*   **Fault Domains:** Implement fault-domain and restart-policy metadata in the process manager for robust recovery.