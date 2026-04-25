---
title: AI Scheduler Status and Roadmap
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
# AI Scheduler Status and Roadmap

## Current status (as implemented)

### Implemented

- AI telemetry context and update routines are present.
- Weak arch hook for PMC sampling is present; deterministic fallback is present.
- Suggestion ingestion queue exists and is drained from scheduler control flow.
- Core actions are wired through scheduler helpers (migrate/priority/throttle paths).

### Partially implemented / hardening needed

- Rich auditability for rejected suggestions is limited.
- End-to-end governor protocol hardening (authn/authz + structured reason codes) needs expansion.
- Explainability/debug tracing for AI decisions is still basic.
- Scheduler policy behavior under high-rate suggestion load needs broader stress coverage.

### Not yet production-hardened

- Formal safety envelopes for each AI action type.
- Deterministic rollback strategy for multi-step AI action sequences.
- Profile-specific validation packs for RT/EDGE/DESKTOP deployments.

## Roadmap

### Near term

1. Add structured reason/status codes for every accepted/rejected suggestion.
2. Add stress tests for queue overflow, adversarial cadence, and cross-core churn.
3. Add tracepoints for telemetry snapshots and action outcomes.

### Mid term

1. Add policy guardrails (per-action bounds, cooldowns, and rollback hooks).
2. Normalize telemetry quality across architectures where PMC availability differs.
3. Strengthen governor-kernel contract tests under mixed policy modes (priority/CFS/EDF).

### Long term

1. Profile-specific AI policy packs with conformance tests.
2. Cluster-level coordination model for multikernel scheduling domains.
