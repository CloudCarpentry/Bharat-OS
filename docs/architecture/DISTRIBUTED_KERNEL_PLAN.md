# Distributed Kernel Ownership Plan

## Objective

This document outlines the concrete refactoring plan to transition Bharat-OS from a shared-state SMP (Symmetric Multiprocessing) architecture with multikernel plumbing toward a true **distributed-kernel** architecture. The goal is evolution, not demolition, keeping the current kernel usable while systematically replacing global shared state with explicit ownership and messaging.

## Guiding Principles

We avoid sprinkling consensus (like Raft) everywhere. Instead, we split the work into three distinct layers:

1.  **Transport layer** — reliable core-to-core messaging.
2.  **Ownership layer** — every global object gets an explicit owner core.
3.  **Consistency layer** — only object types that need coordination get protocol logic.

This approach aligns with multikernel research systems (like Barrelfish): explicit communication, replicated or partitioned state, and object-specific coordination rather than giant global locks.

---

## Phases of Refactoring

### Phase 0 — Stabilize the Current Foundation

**Goal:** Make the current URPC / SMP code robust enough that later distributed ownership does not collapse into a fragile mess.
**Target File:** `kernel/src/ipc/multikernel.c` (currently a transport skeleton with visible core-0 bias).

**Actions:**
*   Introduce a message type table for kernel control messages (`MK_MSG_MEM_RESERVE`, `MK_MSG_PROC_LOOKUP`, etc.).
*   Add request IDs / transaction IDs.
*   Implement ack / nack / timeout / retry mechanics.
*   Replace hardcoded notification-to-core-0 behavior with notification to the actual receiver core.
*   Add a per-channel state machine.
*   Implement a small replay-safe transaction cache to prevent duplicate commits on retransmissions.

**New Companion Files:**
*   `kernel/include/ipc/mk_proto.h`
*   `kernel/include/ipc/mk_txn.h`
*   `kernel/src/ipc/mk_proto.c`
*   `kernel/src/ipc/mk_txn.c`

### Phase 1 — Introduce Ownership Before Distributed Commit

**Goal:** Stop treating global kernel objects as anonymously shared blobs. Establish "Which core owns this object?" before attempting any 2PC.

**New Subsystem:**
*   `kernel/include/multikernel/ownership.h`
*   `kernel/src/multikernel/ownership.c`

**Policy (First Version):** Use deterministic ownership, not consensus.
*   **Physical frames:** Owner determined by NUMA node / frame range.
*   **Process IDs:** Owner determined by hash or creator core.
*   **Address spaces:** Owner is the process owner.
*   **Capabilities:** Owner is the capability-space owner.

### Phase 2 — Refactor PMM First

**Goal:** Physical frame allocation is the cleanest place to eliminate shared global state. The current PMM uses shared allocator structures with locks (SMP-ish semantics).

**Target File:** `kernel/src/mm/pmm.c`

**Actions:** Split PMM into two modes (preserve current SMP path, add distributed ownership path).
*   Break allocator into: local frame cache, owned frame pool, remote reservation path.
*   Add APIs: `pmm_alloc_local()`, `pmm_alloc_owned()`, `pmm_request_remote()`, `pmm_commit_remote()`, `pmm_abort_remote()`.
*   Enforce rule: A non-owner core cannot directly mutate a remote frame bitmap/list.
*   Protocol: Use 2PC-lite for frame reservations (prepare/reserve, commit/free, abort).

**New Files:**
*   `kernel/include/mm/pmm_dist.h`
*   `kernel/src/mm/pmm_dist.c`

### Phase 3 — Refactor Process and Thread Ownership

**Goal:** Currently, execution is partly distributed (per-core runqueues), but identity (PCB/TCB metadata) is globally shared. Separate execution placement from object ownership and migration authority.

**Target File:** `kernel/src/sched.c`

**Actions:**
*   Add `owner_core` to process and thread structs.
*   Replace direct mutation of foreign-owned PCB/TCB fields with local enqueue (if owned here) or remote message (if owned elsewhere).
*   Move slot allocation behind ownership APIs (`proc_create_local()`, `proc_create_remote()`, etc.).
*   Restrict global balancing logic (currently biased to core 0) into a policy module.

**New Files:**
*   `kernel/include/proc/proc_owner.h`
*   `kernel/src/proc/proc_owner.c`
*   `kernel/include/sched/sched_migrate.h`
*   `kernel/src/sched_migrate.c`

### Phase 4 — Refactor VMM / Address-Space Ownership

**Goal:** Transition from treating address-space metadata as shared truth with cross-core shootdowns to an owner-serialized model.

**Target File:** `kernel/src/mm/vmm.c`

**Actions:**
*   Add `owner_core` to address-space/vm-space objects.
*   Separate local page-table edits, remote mapping requests, and distributed invalidation messages.
*   Protocol: Owner validates change, increments mapping epoch, and sends targeted invalidation messages (no global broadcast).

**New Files:**
*   `kernel/include/mm/vmm_dist.h`
*   `kernel/src/mm/vmm_dist.c`
*   `kernel/include/mm/tlb_proto.h`
*   `kernel/src/mm/tlb_proto.c`

### Phase 5 — Capability and Security Object Ownership

**Goal:** Apply capability ownership to avoid chaos across cores.

**Target Files:** Capability system modules (e.g., `kernel/src/cap/`).

**Actions:**
*   Implement capability owner, remote retype requests, revoke broadcasts, reference tracking, and generation numbers/epochs.
*   Protocol: Owner-serialized updates. Revoke is an ordered broadcast from the owner, requiring acks from holders before teardown.

**New Files:**
*   `kernel/include/cap/cap_proto.h`
*   `kernel/src/cap/cap_dist.c`

### Phase 6 — Replace "Master-Core Drift" with Monitor Cores

**Goal:** Eliminate implicit "master core" behavior (especially core 0 bias) at runtime.

**Target Files:** `kernel/src/multicore.c` (restrict to bootstrap), `kernel/src/ipc/multikernel.c` (route via monitors).

**Actions:** Introduce a "monitor role".
*   Responsibilities: ownership directory, heartbeat/liveness, transaction timeout cleanup, migration orchestration.
*   Deployment: one monitor per core, deterministic responsibility per object range.

**New Files:**
*   `kernel/include/multikernel/monitor.h`
*   `kernel/src/multikernel/monitor.c`

### Phase 7 — Add Optional Consensus Only Where Worth the Pain

**Goal:** Introduce consensus protocols (like Raft) *only* for state that absolutely requires it, and *only* after ownership is well-defined.

**Potential use cases:** Global namespace allocation, replicated monitor metadata, distributed service registry.

**New Files (Later):**
*   `kernel/include/multikernel/consensus.h`
*   `kernel/src/multikernel/raft_lite.c`

---

## Work-Breakdown Table

| Module | Exact Responsibilities | API Changes | Risk Level | Test Cases | Migration Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Transport (`multikernel.c`, `mk_proto.*`, `mk_txn.*`)** | Message table, txns, ack/nack, state machine, tx cache, remove core-0 bias. | Add `mk_txn_start()`, `mk_msg_send()`, etc. | **High** | Replay handling, message drop recovery, stress test URPC channels. | Must remain backward-compatible with existing URPC usage initially. |
| **Ownership Base (`ownership.*`)** | Define authority map (who owns what ID/frame). | Add `mk_owner_for_frame()`, `_pid()`, `_asid()`, `_cap()`. | Low | Deterministic hash/range lookups. | Read-only structural addition first. |
| **PMM (`pmm.c`, `pmm_dist.*`)** | Split shared PMM from distributed path. 2PC frame reservation. | Add `pmm_alloc_owned()`, `pmm_request_remote()`. | **High** | Allocate on Node A, request from Node B, abort test. | Put behind `CONFIG_MULTIKERNEL_PMM_DIST` flag. |
| **Sched (`sched.c`, `proc_owner.*`, `sched_migrate.*`)** | Bind PCB/TCB to owner. Cross-core enqueue. Migration protocol. | Add `proc_create_remote()`, `thread_migrate_commit()`. | Medium | Remote thread creation, cross-core wakeup, migration commit. | Do not break local SMP fast-path. |
| **VMM (`vmm.c`, `vmm_dist.*`, `tlb_proto.*`)** | Owner-serialized mapping. Targeted TLB shootdowns. | Add `vmm_map_remote()`, `tlb_invalidate_epoch()`. | **High** | Cross-core map/unmap races, TLB flush validation. | Replaces global IPI broadcasts. |
| **Capabilities (`cap_dist.*`, `cap_proto.*`)** | Epoch-based revoke, distributed retype, reference tracking. | Add `cap_revoke_broadcast()`. | Medium | Revoke mid-invocation across cores. | Capability core must handle async teardowns. |
| **Monitors (`multicore.c`, `monitor.*`)** | Heartbeats, timeout sweeps, directory service. Remove core 0 runtime authority. | Init monitor roles. | Low | Core failure detection, directory handoff. | Restrict `multicore.c` strictly to early boot. |
| **Consensus (`consensus.*`, `raft_lite.*`)** | (Future) Fault-tolerant directories and global names. | TBD | Low (Deferred) | Leader election, log replication. | Only for specific metadata, never general data. |

---

## Implementation Order

**Stage 1 — No Semantic Breakage:**
1. `multikernel.c`
2. `mk_proto.*`
3. `mk_txn.*`
4. `ownership.*`

**Stage 2 — First Real Distributed Object:**
5. `pmm.c`
6. `pmm_dist.c`

**Stage 3 — Process Authority Cleanup:**
7. `sched.c`
8. `proc_owner.c`
9. `sched_migrate.c`

**Stage 4 — Address Spaces:**
10. `vmm.c`
11. `vmm_dist.c`
12. `tlb_proto.c`

**Stage 5 — Security Semantics:**
13. Capability modules
14. Revoke / retype protocols

**Stage 6 — Resilience:**
15. Monitor failover
16. Optional Raft-lite (for monitor metadata only)
