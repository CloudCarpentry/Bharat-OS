# ADR Guide and Index

This directory provides ADR process guidance and cross-links to accepted Bharat-OS decisions.

## ADR locations in this repository

- `docs/decisions/` contains the primary accepted ADR series (`ADR-001` to `ADR-006`).
- `docs/adr/ADR-007-experimental-scope.md` extends that series with explicit v1 vs experimental boundary rules.

## Current accepted ADR index

1. [`docs/decisions/ADR-001-microkernel-vs-hybrid.md`](../decisions/ADR-001-microkernel-vs-hybrid.md)
2. [`docs/decisions/ADR-002-capability-model.md`](../decisions/ADR-002-capability-model.md)
3. [`docs/decisions/ADR-003-multikernel-messaging.md`](../decisions/ADR-003-multikernel-messaging.md)
4. [`docs/decisions/ADR-004-linux-personality-first.md`](../decisions/ADR-004-linux-personality-first.md)
5. [`docs/decisions/ADR-005-ml-stays-out-of-ring-0.md`](../decisions/ADR-005-ml-stays-out-of-ring-0.md)
6. [`docs/decisions/ADR-006-numa-awareness.md`](../decisions/ADR-006-numa-awareness.md)
7. [`ADR-007-experimental-scope.md`](ADR-007-experimental-scope.md)

## ADR authoring expectations

When adding a new ADR:

- Continue numeric ordering (`ADR-008`, `ADR-009`, ...).
- Use clear sections: **Context**, **Decision**, **Consequences**.
- Link the ADR from this index and any relevant architecture pages.
- Prefer one decision per ADR to keep scope reviewable.

## Scope discipline

ADR-007 is the operational boundary for v1 kernel scope.
If a proposed change moves a feature from experimental to core, introduce a dedicated ADR before implementation.
