# Merged Review: `docs/reviews/gap_analysis` + `docs/reviews`

Date: 2026-04-22  
Purpose: Consolidate key findings from both documentation tracks into one actionable review baseline.

## 1) Source sets merged

### Gap analysis documents
- `docs/reviews/gap_analysis/latest_gap_analysis.md`
- `docs/reviews/gap_analysis/layer_reference_gap_analysis_2026-04-19.md`
- `docs/reviews/gap_analysis/interrupt_controller_architecture_plan.md`
- `docs/reviews/gap_analysis/runtime_isa_extension_strategy.md`
- `docs/reviews/gap_analysis/algorithm_improvement/*`

### Review documents
- `docs/reviews/current-implementation-review.md`
- `docs/reviews/main-branch-gap-validation_2026-04-21.md`
- `docs/reviews/architecture_authority_lifecycle_failure_review_2026-04-21.md`
- `docs/reviews/architecture_review_enhancement_plan.md`
- `docs/reviews/context-switching-hw-optimization-review.md`
- `docs/reviews/host-testing-and-qemu-runner-review_2026-04-21.md`
- `docs/reviews/ipc_architecture_profile_readiness_review_2026-03-21.md`
- `docs/reviews/memory-and-fileop-layer-placement-review_2026-04-20.md`
- `docs/reviews/modern_primitives_personality_impact_review_2026-04-20.md`

## 2) Unified status snapshot

Across both folders, the same macro-pattern appears:

1. Architecture direction is strong and mostly consistent.
2. Many subsystem contracts exist but runtime depth/hardening is uneven.
3. Several older docs mixed “missing”, “stubbed”, and “implemented-but-needs-hardening”.
4. Highest-leverage work now is implementation hardening, ownership clarity, and measurable validation.

## 3) Cross-document convergence (what both tracks agree on)

## A. Kernel and service boundaries
- Boundary intent is clear (small kernel mechanism, policy in services).
- Main risk is boundary drift through convenience references and duplicate logic.
- Priority action: enforce layering with checks, ownership, and interface contracts.

## B. IPC and lifecycle correctness
- IPC architecture remains central and mostly sound directionally.
- Readiness risk sits in lifecycle semantics (timeouts, cancellation, fault containment, teardown paths).
- Priority action: tighten lifecycle state models and failure semantics before widening feature set.

## C. Scheduler and context-switch hot paths
- Review notes and algorithm audits both highlight linear-search behavior and hot-path costs.
- Priority action: remove avoidable O(n) scans in scheduler-critical lookups and validate with perf counters.

## D. Memory/file operation placement and fast paths
- Layer placement for memory/file primitives is generally good but not fully normalized.
- Priority action: finish placement cleanup and lock down call-path policy for memcpy/memset/open/openat family.

## E. Interrupt and ISA evolution
- Interrupt architecture plan and ISA-extension strategy are aligned with phased enablement.
- Priority action: preserve portable defaults while adding runtime-selected fast paths with strict fallback behavior.

## F. Testing and host execution confidence
- Testing runner and host path documentation improved, but reliability depends on repeatable gates.
- Priority action: promote review claims into CI-enforced checks and profile-based validation suites.

## 4) Consolidated gap taxonomy

Use this normalized classification in future reports to prevent ambiguity:

- **G0 — Present and validated:** implemented with quality/tests/coverage evidence.
- **G1 — Present but fragile:** implemented, but lacks robustness/observability/perf confidence.
- **G2 — Present as scaffold/stub:** API exists, core behavior partial.
- **G3 — Missing:** required capability absent.
- **G4 — Architectural risk:** capability exists but introduces boundary, authority, or failure-containment concerns.

## 5) Recommended execution order (merged roadmap)

1. **Correctness hardening first**
   - IPC lifecycle, teardown semantics, error propagation, authority checks.
2. **Boundary and ownership enforcement**
   - Layer compliance checks and responsibility matrix across core/kernel/HAL/services.
3. **Performance on proven hotspots**
   - Scheduler lookup paths, context-switch-adjacent costs, checksum/hash/memory hotspots.
4. **Interrupt/ISA phased acceleration**
   - Feature detection + guarded fast paths + deterministic fallbacks.
5. **Documentation freshness loop**
   - Monthly refresh to retire stale assumptions and re-label gaps by G0–G4.

## 6) Deliverables to maintain in `docs/reviews`

To keep the merge sustainable, retain this pattern:

- One rolling merged review (this file, date-stamped updates).
- One evidence table mapping each major claim to code/test artifacts.
- One short changelog section per refresh (newly validated, newly regressed, newly discovered).

## 7) Immediate follow-up checklist

- [ ] Convert major review claims into explicit, automatable acceptance checks.
- [ ] Tag every open gap as G1/G2/G3/G4 with owner + target milestone.
- [ ] Add hotspot microbenchmarks for scheduler lookup and memory/checksum paths.
- [ ] Reconcile layered placement findings into lint/CI policy.
- [ ] Track interrupt/ISA staged rollout with per-architecture fallback tests.

---

This file is the merged baseline requested for `docs/reviews/gap_analysis` and `docs/reviews`, maintained under `docs/reviews`.
