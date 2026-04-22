# Bharat-OS – Critical Gap Analysis & Execution Roadmap

## 1. Executive Summary

Bharat-OS has a strong architectural foundation with a clear direction toward a capability-oriented multikernel OS. However, the current codebase reveals a mismatch between architectural intent and runtime maturity.

This roadmap focuses on **closing correctness, ownership, and runtime gaps first**, before expanding feature surface.

### Core Thesis

> Stabilize kernel invariants + enforce capability boundaries + make services actually run → THEN scale features.

---

## 2. Current State Assessment

### Strengths

* Clean layered architecture (kernel → services → stacks → personalities)
* Strong multikernel vision (per-core ownership, message passing)
* Networking subsystem is most mature (netmgr + netstack)
* Cross-architecture intent (x86_64, ARM64, RISC-V)

### Weaknesses

* Services exist but are not runtime-complete
* Capability system not enforced (only structural)
* Memory subsystem lacks full correctness guarantees
* Scheduler still has shared global state
* Build includes non-functional components (illusion of completeness)

---

## 3. Critical Gaps (Must Fix First)

### 3.1 Kernel Correctness & Invariants

* Global thread/process registries still exist
* TLB shootdown is unbounded and spin-based
* PMM lacks per-core ownership model
* No formal invariants enforced via tests

**Impact:** System instability at scale, breaks multikernel model

---

### 3.2 Capability Enforcement (Security Gap)

* Capability checks allow access with minimal validation
* No strict enforcement of object, rights, or scope

**Impact:** Architecture promise broken → security model invalid

---

### 3.3 Service Runtime Incompleteness

* process_manager and vm_manager are placeholders
* Services do not run full event loops
* No lifecycle management (restart, supervision)

**Impact:** OS cannot function as a real service-oriented system

---

### 3.4 Memory & MMU Discipline

* No unified authority path (fault → aspace → object → HAL)
* Missing per-core allocation strategy
* Weak TLB coordination protocol

**Impact:** Memory correctness issues, SMP instability

---

### 3.5 Architecture Drift

* Duplicate subsystems (legacy vs new)
* Build system includes incomplete services

**Impact:** Confusion, inconsistent behavior, slower development

---

## 4. Execution Strategy

### Guiding Principles

1. Kernel = mechanism only
2. Services = policy + orchestration
3. No shared mutable state across cores
4. All cross-core actions = message-based
5. Every subsystem must have acceptance tests

---

## 5. Phased Roadmap

## Phase 0: Kernel Stabilization (CRITICAL)

### Goals

* Make kernel correct, deterministic, and enforceable

### Tasks

* Remove global scheduler state → per-core ownership
* Replace TLB shootdown with bounded protocol (URPC + ack + timeout)
* Implement per-core PMM caches (magazines)
* Define syscall + capability contract and freeze ABI
* Add invariant test suite (scheduler, memory, IPC)

### Deliverables

* Stable kernel invariants
* Deterministic cross-core behavior
* Passing stress tests

---

## Phase 1: Capability Enforcement

### Goals

* Make capability model real and secure

### Tasks

* Implement capability validation (object + rights + scope)
* Enforce checks at all service IPC boundaries
* Introduce capability lifecycle (create, revoke, transfer)
* Add security test suite

### Deliverables

* End-to-end capability enforcement
* Zero bypass paths

---

## Phase 2: Service Runtime Enablement

### Goals

* Turn services into real OS components

### Tasks

* Implement process_manager (process lifecycle)
* Implement vm_manager (address space lifecycle)
* Add servicemgr (service lifecycle supervisor)
* Convert services to event-driven loops
* Add restart/backoff/watchdog logic

### Deliverables

* Running service-oriented OS
* Fault-tolerant service model

---

## Phase 3: Memory System Completion

### Goals

* Complete a production-grade, profile-aware memory subsystem
* Stabilize Bharat-OS memory behavior across MMU-full, MMU-lite, and MPU-only targets
* Deliver robust ownership, lifecycle, and synchronization semantics for frames, mappings, regions, and device-visible memory

### Tasks

* Implement unified memory contract across MMU-full / MMU-lite / MPU-only
* Complete Advanced VM path for sparse-MMU targets
* Harden Minimal Memory Core for MMU-lite / MPU-only targets
* Add frame, mapping, and region lifecycle management
* Normalize HAL PT behavior using capability-driven profile dispatch
* Integrate TLB synchronization only where translation model requires it
* Implement DMA / IOMMU / MPU-safe device visibility model
* Add NUMA-ready placement and allocation metadata
* Add profile-matrix memory verification suite

### Deliverables

* robust, profile-truthful memory model
* SMP-safe behavior where supported
* explicit degradation path for constrained targets
* test-backed memory subsystem across all supported translation models

> **Important Note for Contributors:** Do not force MMU-full abstractions onto MMU-lite or MPU-only platforms. Use capability-driven behavior from HAL as the source of truth. Unsupported semantics must stay explicitly unsupported, not emulated poorly.

---

## Phase 4: Architecture Alignment

### Goals

* Clean, enforceable architecture

### Tasks

* Remove legacy subsystems
* Align build system with runtime reality
* Enforce folder structure contracts
* Define service boundaries clearly

### Deliverables

* Clean architecture
* No duplication or drift

---

## Phase 5: Feature Expansion (ONLY AFTER ABOVE)

### Goals

* Expand capabilities safely

### Tasks

* Secure boot + OTA
* Accelerator framework (GPU/AI)
* Advanced networking (DPDK, zero-copy)
* Cross-node multikernel messaging
* Personality expansion (Linux/Android)

### Deliverables

* Scalable and production-ready OS

---

## 6. Prioritization Matrix

| Area                   | Priority | Impact    | Risk   | Effort |
| ---------------------- | -------- | --------- | ------ | ------ |
| Kernel invariants      | P0       | Very High | High   | High   |
| Capability enforcement | P0       | Very High | High   | Medium |
| Service runtime        | P1       | High      | Medium | High   |
| Memory system          | P1       | High      | High   | High   |
| Architecture cleanup   | P2       | Medium    | Low    | Medium |
| Advanced features      | P3       | Medium    | Medium | High   |

---

## 7. Acceptance Criteria

### Kernel

* No global mutable shared state
* All cross-core operations message-based
* Deterministic scheduling under load

### Memory

* No leaks or double free under stress
* Correct TLB synchronization
* Fault handling consistent

### Services

* All services run event loops
* Services restart automatically on failure

### Security

* All IPC paths enforce capability checks
* Unauthorized access impossible

---

## 8. Immediate Next Actions (2–4 Weeks)

1. Fix scheduler global state → per-core ownership
2. Redesign TLB shootdown protocol
3. Implement full capability validation engine in netmgr (pilot)
4. Build invariant test harness (scheduler + memory)
5. Convert netmgr into fully running daemon

---

## 9. Jira-Ready Execution Plan

## Epic E0 — Kernel Invariants Hardening

**Objective:** Remove architectural contradictions in scheduler/core ownership and establish deterministic kernel behavior.

### Story E0-S1 — Eliminate global scheduler registries

**Priority:** P0
**Impact:** Very High
**Dependencies:** None

**Tasks**

* Replace global thread registry with per-core owned registries
* Replace global process registry with per-core or service-owned metadata handoff
* Remove linear global TID lookup from fast path
* Introduce remote thread/process lookup through explicit message path
* Update scheduler host/unit tests

**Likely Code Areas**

* `kernel/src/sched/`
* `kernel/include/`
* `kernel/src/tests/`
* `tests/host/` or equivalent scheduler harness

**Acceptance Criteria**

* No global mutable scheduler/process arrays in runtime path
* Remote enqueue/lookup goes through message mechanism
* Existing scheduler tests pass
* New multicore ownership stress tests pass

### Story E0-S2 — Bounded TLB shootdown protocol

**Priority:** P0
**Impact:** Very High
**Dependencies:** E0-S1 preferred

**Tasks**

* Replace spin/mailbox-only coordination with queue/ring-backed protocol
* Add request ID, ack tracking, timeout, and failure reporting
* Support page, range, and full-aspace invalidation
* Add stress tests for concurrent invalidations

**Likely Code Areas**

* `kernel/src/mm/tlb_coordinator.c`
* `kernel/include/hal/`
* `kernel/src/hal/x86_64/`
* `kernel/src/hal/arm64/`
* `kernel/src/hal/riscv64/`
* `tests/mm/`

**Acceptance Criteria**

* No unbounded waiting in shootdown path
* Multicore TLB invalidation tests pass under stress
* Architecture-specific backends respect shared contract

### Story E0-S3 — PMM ownership and per-core magazines

**Priority:** P0
**Impact:** High
**Dependencies:** None

**Tasks**

* Add per-core page magazines/caches
* Define remote free semantics
* Formalize refcount ownership transitions
* Add leak/double-free stress tests

**Likely Code Areas**

* `kernel/src/mm/pmm/`
* `kernel/src/mm/`
* `kernel/src/tests/`
* `tests/mm/`

**Acceptance Criteria**

* Per-core fast allocation path exists
* Remote free path is explicit and test-covered
* No leak/double-free in stress harness

---

## Epic E1 — Capability Enforcement Completion

**Objective:** Convert the capability model from structural intent into enforced runtime security.

### Story E1-S1 — Pilot strict checks in netmgr

**Priority:** P0
**Impact:** Very High
**Dependencies:** None

**Tasks**

* Replace basic fail-closed shim with full object/right/scope validation
* Maintain deny-by-default behavior (implemented via shim)
* Add audit logging for capability failures
* Add negative-path tests for unauthorized IPC

**Likely Code Areas**

* `services/netmgr/src/capability_checks.c`
* `services/netmgr/src/ipc_dispatch.c`
* `services/netmgr/tests/`

**Acceptance Criteria**

* Unauthorized operations are rejected
* Tests cover valid, invalid, stale, and over-scoped capabilities

### Story E1-S2 — Roll out capability enforcement framework across services

**Priority:** P0
**Impact:** High
**Dependencies:** E1-S1

**Tasks**

* Create reusable capability validation library/helper
* Enforce checks in all IPC entry points
* Add revocation/transfer lifecycle handling

**Likely Code Areas**

* `lib/`
* `services/*/`
* `idl/`
* `uapi/`

**Acceptance Criteria**

* All service IPC boundaries have explicit capability checks
* Revoked capabilities fail predictably

---

## Epic E2 — Service Runtime Completion

**Objective:** Make Bharat-OS operate like a real service-oriented system instead of a stubbed layout.

### Story E2-S1 — Implement process_manager runtime [COMPLETED]

**Priority:** P1
**Impact:** High
**Dependencies:** E1 foundation preferred

**Tasks**

* ~~Define process lifecycle state machine~~
* ~~Implement IPC handlers for create/start/stop/query~~
* ~~Add integration with scheduler and service manager~~

**Likely Code Areas**

* `services/process_manager/`
* `idl/`
* `uapi/`

**Acceptance Criteria**

* ~~process_manager no longer returns as placeholder main~~
* ~~Process lifecycle operations are callable and test-covered~~

### Story E2-S2 — Implement memory runtime by profile [COMPLETED]

**Priority:** P1
**Impact:** High
**Dependencies:** E0-S2, E0-S3

**Tasks**

* ~~Define address-space lifecycle contract~~
* ~~Implement map/unmap/protect/query flows~~
* ~~Integrate with kernel VM path~~

**Likely Code Areas**

* `services/vm_manager/`
* `services/memdom_manager/`
* `kernel/src/mm/vm/`
* `kernel/src/mm/mpu/`
* `idl/`
* `uapi/`

**Acceptance Criteria**

* ~~vm_manager supports real lifecycle operations~~
* ~~Mapping tests pass end to end~~

### Story E2-S3 — Add service supervisor runtime [COMPLETED]

**Priority:** P1
**Impact:** High
**Dependencies:** E2-S1, E2-S2

**Tasks**

* ~~Add event loop, restart policy, backoff, crash tracking~~
* ~~Add service health model and watchdog hooks~~
* ~~Add boot ordering/service dependency handling~~

**Likely Code Areas**

* `services/servicemgr/` or equivalent
* `services/`
* `boot/`

**Acceptance Criteria**

* ~~Managed services restart on failure~~
* ~~Boot order and dependencies are enforced~~

---

## Epic E3 — Profile-Aware Memory System Completion

**Objective:** Complete Bharat-OS memory as a profile-aware subsystem that works correctly across MMU-full, MMU-lite, and MPU-only targets.

### E3 Priority gap closure requirements (authoritative)

**Current strengths to preserve**

* Explicit memory model API already exists with capability distinctions across MMU-full / MMU-lite / MPU.
* Arch capability reporting is present on major targets and already encodes model diversity.
* VM space object timing behavior (`prefault`, lazy realization, runtime PT allocation gating) is already surfaced.

**Production closure requirements**

* Boot-time runtime memory model validation must be authoritative (`arch_caps × selected profile × required guarantees`) with fail-closed semantics.
* VM distributed map/unmap/realize/destroy ownership flow must be monitor-confirmed with rollback safety.
* MMU-lite/MPU profiles must publish strict unsupported-feature contracts and enforce them (no implicit MMU-full emulation).
* IOMMU/NUMA/hugepage paths must be profile-matrix gated so unsupported paths are disabled and cannot be accidentally invoked.

### Story E3-S1 — Implement unified memory contract across MMU-full / MMU-lite / MPU

**Priority:** P1
**Impact:** High
**Dependencies:** E0-S2, E0-S3, E2-S2

**Tasks**

* Introduce a top-level memory profile dispatcher driven by `hal_pt_caps()` / translation kind / exec class
* Add runtime verification layer for `mem_model_get_current()` outputs against boot-detected hardware capabilities
* Add fail-closed boot behavior when required memory guarantees are not met by detected hardware
* Implement MMU-full path (`request/fault -> aspace -> region -> vm_object -> hal_pt -> tlb`)
* Implement MMU-lite path (`request/prefault/realize -> aspace -> region -> vm_object -> hal_pt or fixed-map backend -> limited tlb sync`)
* Implement MPU path (`request/protection-domain update -> region set -> hal_mpu/prot_domain programming -> synchronization`)
* Make `vm_handle_fault()` callable only on profiles that advertise meaningful fault support
* Remove assumptions that every region ends in `map_page()`

**Likely Code Areas**

* `kernel/src/mm/vm/fault/`
* `kernel/src/mm/vm/aspace/`
* `kernel/src/mm/vm/objects/`
* `kernel/src/hal/*/`
* `docs/architecture/memory/`

**Acceptance Criteria**

* Dispatch logic correctly routes based on HAL capabilities
* Sparse VM operations are separate from eager/static realization operations and region-protection operations
* No fake page-fault expectations on platforms that cannot support them truthfully

### Story E3-S2 — Frame + mapping + region lifecycle management

**Priority:** P1
**Impact:** High
**Dependencies:** E3-S1

**Tasks**

* Define physical frame lifecycle (boot-known, free, allocated, pinned, reclaimable, quarantined/poisoned, device-owned / DMA-exported)
* Define sparse mapping lifecycle (reserved, attached to object, realized, protected, unmapped, destroyed)
* Remove deprecated realization walk pathways and quarantine-only fallback paths
* Make distributed destroy path include authoritative remote realization cleanup with explicit completion ACK
* Define region / protection lifecycle (declared, validated, programmed, active, reprogram pending, revoked)

**Likely Code Areas**

* `kernel/src/mm/`
* `kernel/src/mm/pmm/`
* `docs/architecture/memory/`

**Acceptance Criteria**

* MPU platforms correctly utilize the region/protection lifecycle
* PMM correctly handles the physical frame lifecycle

### Story E3-S3 — Implement DMA / IOMMU / MPU-safe device visibility model

**Priority:** P1
**Impact:** Medium-High
**Dependencies:** E3-S1, E3-S2

**Tasks**

* Define pinned page ownership and contiguous allocation path
* Differentiate coherent vs non-coherent DMA
* Implement software cache maintenance for MMU-lite / MPU-only
* Add profile-level call-path guards that hard-disable unavailable IOMMU semantics
* Define direct DMA fallback policy when no IOMMU exists
* Implement IOVA domain lifecycle when IOMMU exists
* Define device isolation levels (unmanaged, identity, bypass, managed, blocked)

**Likely Code Areas**

* `kernel/src/mm/iommu/`
* `kernel/include/hal/`
* `drivers/`
* `docs/architecture/memory/`

**Acceptance Criteria**

* Drivers must not assume `physical address == safely DMA-visible address`
* MPU-only gets a valid DMA contract (even if software-synchronized and region-based)

### Story E3-S4 — Add NUMA-ready placement contract

**Priority:** P2
**Impact:** Medium
**Dependencies:** E3-S1

**Tasks**

* Establish node metadata as authoritative kernel objects
* Add explicit no-NUMA/no-hugepage profile declarations and enforce hard-disabled behavior
* Define allocation preferences and locality tagging on VM objects / DMA buffers
* Implement node affinity hints in PMM and aspace
* Ensure clean no-op behavior on single-node systems

**Likely Code Areas**

* `kernel/src/mm/pmm/`
* `kernel/src/mm/vm/aspace/`

**Acceptance Criteria**

* NUMA metadata is correctly plumbed
* Single-node systems fallback gracefully without performance penalty
* (Note: Full NUMA balancing/migration is explicitly deferred)

### Story E3-S5 — Add profile-matrix memory verification suite

**Priority:** P1
**Impact:** High
**Dependencies:** E3-S1, E3-S2, E3-S3

**Tasks**

* Create MMU-full tests (faults, COW, TLB invalidation, shared mappings)
* Create MMU-lite tests (eager realization, no-demand-fault builds, fixed mapping stability)
* Create MPU-only tests (region overlap validation, transition, no sparse-page API leakage)
* Create Cross-profile tests (frame lifecycle, DMA mapping lifecycle, NUMA metadata plumbing)

**Likely Code Areas**

* `tests/mm/`
* `kernel/src/tests/`

**Acceptance Criteria**

* Tests pass across all three translation profiles (MMU-full, MMU-lite, MPU-only)
* Explicit unsupported-return tests succeed for paging features on MPU-only targets

---

## Epic E4 — Architecture Cleanup and Drift Control

**Objective:** Align build, runtime, and repo structure with actual implementation truth.

### Story E4-S1 — Remove or quarantine legacy duplicates

**Priority:** P2
**Impact:** Medium
**Dependencies:** E2 network runtime stability

**Tasks**

* Mark legacy services/subsystems deprecated
* Remove duplicate runtime wiring where replacements exist
* Update build defaults to reflect supported paths only

**Likely Code Areas**

* `services/CMakeLists.txt`
* `stacks/`
* legacy `net/` vs `netmgr` / `netstack`

**Acceptance Criteria**

* Default build does not imply unsupported runtime completeness
* One primary implementation path per subsystem

### Story E4-S2 — Contributor architecture contract

**Priority:** P2
**Impact:** Medium
**Dependencies:** None

**Tasks**

* Define what belongs in kernel vs services vs stacks vs personalities
* Define cross-core messaging rule
* Define acceptance-test requirement for architectural changes

**Likely Doc Areas**

* `docs/architecture/`
* `docs/dev/`
* contributor guidelines

**Acceptance Criteria**

* New contributors have enforceable placement and ownership rules
* PR review can reference one architecture contract doc

---

## 10. Dependency-Ordered Delivery Plan

### Sprint Block A — Foundation (Highest Urgency)

* E0-S1 Eliminate global scheduler registries
* E0-S2 Bounded TLB shootdown protocol
* E1-S1 Pilot full capability enforcement in netmgr
* E0-S3 PMM ownership and per-core magazines

### Sprint Block B — Runtime Legitimacy

* E1-S2 Capability enforcement framework rollout
* E2-S1 process_manager runtime
* E2-S2 Implement memory runtime by profile
* netmgr daemonization completion

### Sprint Block C — Platform Cohesion

* E2-S3 service supervisor runtime
* E3-S1 Implement unified memory contract across MMU-full / MMU-lite / MPU
* E3-S2 Frame + mapping + region lifecycle management
* E4-S2 Contributor architecture contract

### Sprint Block D — De-risked Expansion

* E3-S3 Implement DMA / IOMMU / MPU-safe device visibility model
* E3-S4 Add NUMA-ready placement contract
* E3-S5 Add profile-matrix memory verification suite
* E4-S1 Remove legacy duplicates
* selective advanced features only after all above are green

---

## 11. Suggested Priority Stack for Coding Order

### Top 5 coding priorities

1. Scheduler per-core ownership
2. TLB shootdown redesign
3. netmgr full capability enforcement
4. PMM per-core magazines + ownership tests
5. process_manager / memory runtime implementation by profile

### Top 5 documentation priorities

1. Kernel invariants contract
2. Capability model contract
3. Profile-aware memory contract
4. Service lifecycle contract
5. Contributor architecture placement rules

### Top 5 test priorities

1. Multicore scheduler ownership tests
2. TLB invalidation stress tests
3. Capability negative-path tests
4. PMM leak/double-free stress tests
5. End-to-end service lifecycle tests

---

## 12. Delivery Governance Model

### Definition of Done for each story

A story is complete only when all are true:

* Code implemented
* Unit tests added/updated
* Stress/e2e tests added where relevant
* Architecture docs updated
* Build/runtime wiring updated
* Legacy path impact assessed

### Release Gates

**Gate 1 — Kernel Trustworthy**

* Scheduler ownership fixed
* TLB protocol bounded
* PMM ownership tested

**Gate 2 — Security Enforced**

* Capability enforcement active across IPC boundaries

**Gate 3 — Runtime Real**

* process_manager, appropriate memory runtime (based on profile), and service supervisor operational

**Gate 4 — Architecture Clean**

* Legacy duplication reduced
* Build matches supported reality

---

## 13. Final Recommendation

Do NOT expand features right now.

Focus exclusively on:

* Kernel correctness
* Capability enforcement
* Memory discipline
* Service runtime

Once these are solid, Bharat-OS becomes a real platform—not just an architecture.

---

**End of Document**
