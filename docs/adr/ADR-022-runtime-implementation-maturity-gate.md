---
title: 'ADR-022: Runtime implementation maturity gate'
status: Accepted
owner: Team
last_updated: '2024-01-01'
tags:
- doc
see_also:
- none
---

# ADR-022: Runtime implementation maturity gate

## Decision

The versioned implementation maturity manifest is the authority for runtime implementation truth.
Build tooling rejects `STUB` and `TEST_ONLY` components from release and hardened targets before
configuration. Development targets must opt in explicitly. Source scanning may produce diagnostics
but cannot replace the reviewed manifest.

## Consequences

Production builds fail closed while known placeholders exist. Maturity changes require evidence and
review of both the manifest and implementation; they are not inferred from names or return values.
