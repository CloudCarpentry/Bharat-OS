# ADR-009: Documentation Status Labels and Research-Influenced Claims

## Status

Accepted

## Context

Bharat-OS documentation includes both implemented kernel baseline and aspirational architecture. As the project references established research (e.g., multikernel and capability-system literature), docs can drift into ambiguous wording that looks like feature completion claims.

This causes confusion for contributors and reviewers when evaluating project readiness across device categories.

## Decision

Adopt explicit status labeling and claim discipline across README and architecture docs:

1. Every major feature area must distinguish at least:
   - **Implemented baseline**, and
   - **Deferred / roadmap**.
2. Device/use-case sections must map present capability and gaps rather than imply production readiness.
3. References to external research (Barrelfish, seL4, L4-family, AI scheduling literature) must be phrased as **architectural influence**, not equivalence or parity.
4. Architecture index pages should link dedicated docs for:
   - device profile/use-case mapping,
   - AI scheduler implementation status and roadmap.

## Consequences

### Positive

- Reduces ambiguity between current code and future direction.
- Improves planning quality and contributor onboarding.
- Preserves scientific grounding without overstating implementation depth.

### Negative

- Adds maintenance overhead to keep status labels current.
- Requires periodic review when implementation catches up with roadmap items.
