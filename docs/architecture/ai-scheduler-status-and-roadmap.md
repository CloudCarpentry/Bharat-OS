# AI Scheduler Status and Roadmap

This document describes what is currently implemented for the AI-assisted scheduler path and what remains before production-grade deployment.

## Design principle

Bharat-OS keeps complex AI policy in user space and preserves bounded kernel mechanisms. This follows the repository ADR direction that ML policy should remain outside ring-0 while the kernel enforces deterministic action handling.

## Implemented baseline

- **Telemetry and suggestion flow exists end-to-end:**
  - telemetry is sampled in the scheduler path,
  - AI suggestions are ingested through a kernel bridge,
  - scheduler applies bounded action handlers.
- **Capability-protected control plane:**
  - governor endpoint creation and suggestion ingestion are capability checked.
- **Action categories currently wired:**
  - task migration,
  - priority adjustment,
  - core throttle hints.
- **Pluggable contract across profiles and architectures:**
  - architecture-specific PMCs can be sampled via override hooks,
  - deterministic fallback telemetry path exists when PMCs are unavailable.

## Current constraints

- Scheduler runtime still relies on baseline/static structures.
- Full per-core runqueue behavior and deep SMP balancing are not complete.
- Hardware context-switch depth across all target architectures is still limited.
- Telemetry quality and explainability/audit tooling need expansion.

## Roadmap

### Near-term

1. Strengthen per-core scheduling structures and migration policy quality.
2. Expand architecture PMU/PMC integrations and normalize metrics.
3. Add richer tracepoints and audit logs for AI decisions.

### Mid-term

1. Add policy safety envelopes (hard bounds and rollback behavior).
2. Integrate stronger verification and regression tests for AI action effects.
3. Improve operator-facing observability in governor<->kernel interactions.

### Long-term

1. Profile-specific scheduling policies with shared verification contracts.
2. Cluster-level coordination for multikernel/domain scheduling.

## Relationship to ADRs

- ADR-005 defines policy/mechanism split for ML-related behavior.
- ADR-008 defines plugin contract constraints for architecture/profile portability.
- ADR-009 (documentation truthfulness) constrains how roadmap claims are communicated.
