---
title: Bharat-OS Architecture Review Refresh: Authority, Lifecycle, and Failure Containment
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - reviews
see_also:
  - README.md
---
# Bharat-OS Architecture Review Refresh: Authority, Lifecycle, and Failure Containment

**Date:** 2026-04-21  
**Supersedes:** `architecture_authority_lifecycle_failure_review_2026-03-24.md`  
**Scope:** Re-validation of the March review against current architecture docs and main-branch review artifacts.

## Why this refresh exists

The March 24, 2026 review correctly identified major architectural risks, but the repository has moved forward with explicit contracts in multiple critical areas. This refresh separates:

1. **Concerns now covered by defined contracts**
2. **Concerns still relevant and requiring implementation hardening**
3. **Updated, narrower backlog aligned to current state**

---

## Executive Reassessment

### What has materially improved since the previous review

The following are no longer “missing” at architecture-contract level:

- **Endpoint generation / stale-handle model** is now explicitly documented (service identity + incarnation model).
- **DMA + IOMMU lifecycle semantics** are now explicitly documented (ownership transfer, map/sync/unmap lifecycle, isolation requirements).
- **Grant-style DMA authority language** exists in capability architecture docs (lease/revocation framing).
- **Profile and lifecycle governance direction** has stronger documented shape across runtime/init/service lifecycle contracts.
- **Main-branch validation artifacts** already classify prior external gap reports as partially outdated and distinguish “absent” from “present-but-needing-depth”.

### What remains true (and still high priority)

The core warning from the March review remains valid:

> Bharat-OS direction is strong, but **production assurance depends on enforcing contracts in code, tests, and runtime policy** (not only documenting them).

---

## Status by original concern area

## 1) Authority boundaries and capability semantics

**Status:** **Partially addressed, still active risk.**

- Capability architecture and delegation/revocation direction is documented.
- However, repository-wide production guarantees (temporal semantics, stronger provenance/audit depth, and retype/lifecycle invariants) are still identified as pending hardening in current review material.

**Conclusion:** Still relevant, but should be reframed from “missing model” to “hardening + enforcement gap”.

## 2) Endpoint identity, generation safety, and stale-handle rejection

**Status:** **Architecturally addressed.**

- The identity/incarnation model now specifies stable service identity plus generation checks for restart safety.

**Conclusion:** This item should be removed from “missing fundamentals” and tracked under implementation/testing completeness.

## 3) Shared memory and DMA revocation/lifecycle

**Status:** **Contract-present, implementation depth still critical.**

- DMA/IOMMU lifecycle and ownership transitions are documented.
- Grant/lease framing for DMA authority exists.
- Main-branch validation still flags IOMMU enforcement depth and DMA isolation test maturity as priority work.

**Conclusion:** Still highly relevant, but now a “depth + verification” problem rather than an undefined architecture problem.

## 4) Profile system as enforceable policy (not toggles)

**Status:** **Direction improved, enforcement maturity pending.**

- Profile docs and lifecycle contracts exist across architecture docs.
- Remaining challenge is strict runtime enforcement and profile-specific admissibility/transition controls.

**Conclusion:** Still relevant.

## 5) Accelerator integration governance

**Status:** **Improved architecture framing; still a hardening area.**

- Compute architecture documents capability mediation, memory/IOMMU coupling, and queue/completion pathways.
- Remaining work is robust admission control, fault integration, and profile-specific isolation guarantees in implementation.

**Conclusion:** Relevant, but narrower than before.

## 6) Fault containment and restart policy

**Status:** **Conceptual structure present, operational depth pending.**

- Lifecycle/fault ownership appears in service/process architecture.
- Still needs stronger dependency-aware containment, restart policy realization, and evidence via tests.

**Conclusion:** Still relevant and high impact.

## 7) Scheduler/resource governance for mixed criticality

**Status:** **Partially addressed; still relevant.**

- Scheduler architecture and mixed-criticality direction are documented.
- Production-grade admission, bounded interference, and stress-validated budget enforcement remain an active backlog area.

**Conclusion:** Still relevant.

---

## Updated risk ranking (2026-04-21)

### High

1. **IOMMU/DMA enforcement depth + revocation reliability + isolation tests**
2. **Fault-domain containment/restart semantics realized in runtime behavior**
3. **Capability lifecycle hardening (temporal validity, provenance, invariants)**
4. **Cross-core/cross-node IPC reliability semantics (ack/completion/backpressure under load)**

### Medium

5. **Profile policy enforcement depth (runtime mutability and privilege transition guardrails)**
6. **Mixed-criticality budget enforcement (CPU/queue/DMA/interrupt/accelerator)**
7. **Accelerator tenancy and recovery behavior across profiles**

### Low (strategic)

8. **Doc consolidation and drift control across overlapping architecture specs**
9. **Consistent status tagging (spec-only vs implemented vs validated)**

---

## Replacement for the old “immediate freeze candidates”

The previous four-contract freeze recommendation remains useful, but should be updated as follows:

1. **Capability Lifecycle Contract** (include temporal validity + audit provenance requirements)
2. **IPC/URPC Reliability Contract** (identity/generation + ordering + retry/idempotence + backpressure)
3. **DMA/IOMMU Enforcement Contract** (grant lifecycle + revoke completion + profile constraints)
4. **Fault Containment Contract** (dependency graph + restart domains + degraded-mode transitions)

Each contract should now include **three gates**:
- **Spec gate** (documented and versioned)
- **Implementation gate** (core/kernel/service behavior merged)
- **Validation gate** (stress + fault-injection + profile matrix tests)

---

## Practical next steps

1. Convert major contracts to explicit “status blocks” (`spec`, `implemented`, `validated`) in docs.
2. Add test-matrix linkage from each contract to concrete host/sim/hardware checks.
3. Prioritize IOMMU/DMA and fault-domain validation as the first production-hardening wave.
4. Use main-branch gap-validation style for future review refreshes to avoid repeating outdated claims.

---

## Final verdict

The March 24 review is **not fully obsolete**, but it is **too broad and partially stale** for the current repository state.

- Keep its core thesis: authority/lifecycle/failure contracts are central.
- Drop claims that these areas are undefined at architecture level.
- Focus execution on enforcement depth, runtime guarantees, and validation evidence.

This document replaces the previous review as the current baseline.

---

## Addendum (2026-04-22): IPC/URPC Reliability Progress
Following this review, the **IPC/URPC Reliability Contract** has advanced from a pure scaffold to a **partial implementation** gate.
- A fixed-size transaction registry has been added to `mk_proto.c`.
- `ACK`/`NACK` completions are now routed and tracked by transaction ID.
- Bounded timeouts transition expired transactions to a timeout state.
- **Validation gate** remains open pending stress/fault-injection tests.
