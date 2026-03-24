# Bharat-OS Architecture Review: Authority, Lifecycle, and Failure Containment

**Date:** 2026-03-24  
**Scope:** Consolidated critical architecture review across profile-driven multikernel direction, capability model, URPC/service model, and accelerator integration.

## Executive Summary

Bharat-OS is currently strongest in **modularity direction** (small kernel core, profile-oriented composition, service-first evolution), but under-specified in the contracts that determine whether modularity is secure and operationally reliable:

- authority boundaries
- lifecycle ownership
- memory-grant semantics
- revocation guarantees
- endpoint identity/generation safety
- accelerator isolation and admission control
- profile-enforced security posture
- fault containment and restart policy
- mixed-criticality separation

The present design is therefore best described as **architecture-positive but contract-incomplete**: structurally promising, but insufficiently hardened for production-grade multikernel deployment.

## Overall Verdict

### What is directionally correct

1. Small stable core kernel
2. Profile-driven composition
3. Service-oriented advanced functionality (including accelerators)

### What is currently missing

The design language needs to shift from “what modules exist” to “**who has authority over which object, for how long, with what revocation/fault behavior**.”

This is the primary gap between a good research architecture and a durable production architecture.

## Strongest Elements in the Current Direction

- Reusable primitives across deployment classes (cloud/edge/mobile/safety): profile filtering, message discipline, memory classes, timers, power hooks, and fault-domain concepts.
- Accelerator-aware future direction (AES/NPU/FPGA/GPU backends) as a service extension point.

These are strong foundations **if and only if** they are governed by explicit security, ownership, and recovery contracts.

## Critical Finding: Missing Object Authority and Lifecycle Contracts

Every major object class must carry explicit authority and lifecycle rules:

- endpoint
- capability
- thread
- address space
- VM object
- shared buffer
- DMA mapping
- device queue
- service registration record
- profile selector
- accelerator job
- fault domain

Without this, the system accumulates hidden global assumptions despite modular code boundaries.

## Deep Critique and Required Corrections

### 1) Multikernel model: concept strong, semantics incomplete

Current risk: distributed structure without strict distributed semantics.

Required protocol contract for cross-core operations:

- message ID
- source endpoint identity
- target endpoint identity
- generation number
- lease/expiry
- correlation ID
- ordering model
- completion class (fire-and-forget / acked / durable / cancelable)

Also define ownership for retries, timeout handling, idempotence, and dead-destination behavior.

---

### 2) Capability model: likely too coarse

Current risk: broad capabilities that undermine least privilege.

Capabilities must encode:

- object type
- operation rights
- scope
- lifetime
- transferability
- revocability
- profile constraints
- security label/domain tag where needed

Example: accelerator access should split rights into submit, map device-visible buffer, reserve capacity, cancel job, read telemetry, administer backend, register completion endpoint.

---

### 3) Shared memory and DMA: highest-risk zone

Current risk profile:

- stale data leakage
- use-after-free on descriptors
- pinned-page ownership confusion
- DMA escape outside grant window
- completion writes into reused buffers

Required model: **grant-based memory sharing**

- explicit owner and borrower roles
- time-bounded grants
- rights: read/write/device/DMA visibility
- revocation invalidates all dependent mappings
- secure-class zero-on-release
- completion path must re-validate grant

No raw pointers or ad hoc offsets across trust boundaries.

---

### 4) Profile-driven architecture: too configuration-oriented today

Current risk: profiles acting as feature toggles, not security contracts.

Profiles must enforce behavior and security policy, including:

- legal privilege transitions
- allowed sharing models
- overcommit policy
- mixed best-effort vs hard-deadline coexistence
- runtime mutability limits

Examples:

- **Cloud:** mandatory IOMMU for external DMA, no anonymous service registration, strict tenant accounting.
- **Safety:** no dynamic service loading in normal mode, static capability manifests, deterministic scheduling for critical domains.
- **Mobile:** power/thermal QoS policy, trusted path for sensitive devices, strict sensor permission envelope.

---

### 5) Accelerator integration: too compute-centric unless reframed

Current risk: backend implementation concerns (HLS/CUDA/AXI/perf) overshadow OS control concerns.

Accelerator interface must be defined as five contracts:

1. service contract
2. security contract
3. scheduling contract
4. memory contract
5. recovery contract

Performance optimization is secondary to those constraints.

---

### 6) Fault domains: good instinct, insufficient operational depth

Current risk: labeling without containment semantics.

Required capabilities:

- hierarchical fault-domain tree
- dependency edges
- restart containment rules
- state transition policies
- audit trail
- quarantine mode
- degraded-mode contracts

Suggested fault-domain classes:

- kernel-critical
- platform-critical
- security-critical
- tenant-isolated
- safety-critical
- best-effort

Each class needs explicit fault-action matrix (restart only / restart+revoke children / isolate+drain queues / safe-profile switch / operator gate).

---

### 7) Scheduler model: thread-centric scheduling is insufficient

Current risk: service-level interference in a multikernel/service system.

Resource governance must include:

- CPU time
- memory class pressure
- queue budget
- DMA budget
- interrupt budget
- accelerator reservation
- criticality class
- power/thermal budget

---

### 8) Service registration and identity: likely under-secured

Current risk: name/location-based trust assumptions.

Required registration posture:

- static capability manifests now; signed/measured manifests as target state
- stable service ID + instance ID
- generation bump on restart
- profile-bound registration policy
- explicit dependency declaration
- endpoint namespace ownership rules

---

### 9) Mixed-criticality: must be first-class

Current risk: control-plane and best-effort pathways sharing resources by convention.

Must be structurally impossible by default for:

- infotainment/user domains to influence safety control paths
- optimization/AI workloads to block timer-critical control loops
- best-effort ML jobs to consume reserved DMA/queue pools
- cloud telemetry traffic to share hard-deadline pathways

## Risk Ranking

### High

1. Shared memory + DMA without grant lifecycle/revocation
2. Coarse capabilities and weak service identity semantics
3. Missing endpoint generation semantics and stale-handle rejection
4. Fault domains without containment/restart dependency model

### Medium

5. Profile system as config-only rather than policy-enforcing
6. Scheduler/governance model lacking queue/DMA/interrupt budgets
7. Accelerator path defined as performance-first rather than policy-first

### Low (but strategically important)

8. Transitional abstractions and host-test workarounds masking architecture debt
9. UAPI/internal boundary drift (wrappers, duplicate enums, guard mismatches)

## Target-State Contract Set (Immediate Architecture Freeze Candidates)

1. **Capability Contract**
   - canonical object-right schema
   - transfer/revoke semantics
   - profile constraints

2. **IPC/URPC Contract**
   - framing, identity, generation, ordering, completion model
   - status/error model
   - retry/idempotence requirements

3. **Memory Grant + DMA Contract**
   - owner/borrower/grant lifecycle
   - revoke semantics and map/unmap accounting
   - secure zeroization and stale completion protection

4. **Fault Domain + Restart Contract**
   - hierarchy/dependencies
   - containment actions
   - degraded-mode transitions and audit requirements

These four contracts should be treated as architecture baseline gates before expanding subsystem breadth.

## Phased Remediation Plan

### Phase 0 — Contract Baseline (Immediate)

- Publish authority matrix for all major object classes.
- Freeze canonical IPC/URPC header contract.
- Introduce strict endpoint generation checks and stale-handle rejection.
- Define error/status taxonomy for service and cross-core calls.

### Phase 1 — Security and Ownership Hardening

- Implement grant-based shared memory and DMA lifecycle accounting.
- Split broad capabilities into narrow operation rights.
- Enforce bounded message lengths and structured parser validation.
- Add capability and grant audit events.

### Phase 2 — Operational Containment

- Implement hierarchical fault domains with dependency-aware restart policy.
- Add queue/DMA/interrupt budgets and admission control.
- Add profile-enforced registration and privilege-transition policy.

### Phase 3 — Mixed-Criticality and Accelerator Governance

- Encode criticality separation in scheduler and resource allocator.
- Move accelerator framework to policy-governed contract model.
- Add per-profile accelerator tenancy/isolation and recovery semantics.

## Concrete Repository Impact Areas (Implementation Backlog Anchors)

- `docs/architecture/kernel/capabilities/` — capability contract schema, narrow rights model, revocation semantics.
- `docs/architecture/kernel/urpc/` and `docs/architecture/kernel/ipc/` — canonical message framing, endpoint generation, completion taxonomy.
- `docs/architecture/core/` and memory/driver architecture docs — grant-based sharing and DMA lifecycle model.
- `docs/profiles/` and profile-related architecture docs — policy-contract profile definitions.
- fault-domain/restart and scheduler docs — containment rules, budget dimensions, mixed-criticality enforcement.

## Security-Hardening Baseline Checklist

- endpoint generation checks
- bounded message lengths
- strict parser validation
- stale-handle rejection
- secure-buffer zeroization policy
- queue quotas
- capability audit log
- DMA map/unmap accounting

## Final Conclusion

Bharat-OS has a strong foundational direction, but it is currently **stronger on modular decomposition than on authority semantics**. The architectural center of gravity now needs to move decisively to explicit authority, lifecycle, revocation, and fault contracts.

The highest-value immediate move is to lock and enforce four contracts:

1. capability
2. IPC/URPC
3. memory grant + DMA
4. fault-domain/restart

Once those are made canonical, subsequent subsystem growth is far less likely to produce hidden fragility.
