---
title: ADR-015 Documentation Information Architecture and Archive Policy
status: Accepted
owner: Architecture Team
reviewers: Core Maintainers
version: 1.0
last_updated: 2026-04-25
tags:
  - documentation
  - governance
  - architecture
  - repository-structure
---

# ADR-015: Documentation Information Architecture and Archive Policy

## Context

The `docs/` tree accumulated overlapping indexes, stale links, and legacy planning notes that referenced pre-consolidation paths. This caused three recurring problems:

1. **Navigation friction:** top-level documentation indexes pointed to removed or moved files.
2. **Code/document drift:** architecture docs did not consistently map to canonical implementation roots (`core/*`, `interface/*`, `quality/*`).
3. **Low-signal historical artifacts:** backup or superseded files remained in active architecture paths and obscured current guidance.

The repository now has a clearer code layout with canonical roots, so documentation structure must reflect that layout.

## Decision

1. Keep `docs/README.md` as the canonical entry point and align it to the current active subtrees.
2. Maintain `docs/architecture/README.md` as a curated architecture reading order with only valid links.
3. Introduce and maintain `docs/architecture/repository-code-map.md` as the required bridge between architecture documentation and real code folders.
4. Create `docs/archive/` as the standard location for superseded, brainstorm-only, or backup artifacts.
5. When architecture boundaries change, update docs and ADRs together in the same change set.

## Consequences

### Positive

- Faster onboarding: contributors can traverse docs using stable indexes.
- Better traceability: architecture claims are easier to verify against concrete code paths.
- Cleaner active docs: historical artifacts remain accessible without polluting active guidance.

### Trade-offs

- Slightly higher documentation maintenance burden for boundary-changing PRs.
- Requires periodic curation to ensure archived documents remain appropriately categorized.

## Implementation notes

- Active indexes were updated in `docs/README.md` and `docs/architecture/README.md`.
- Legacy/backup architecture files were moved into `docs/archive/`.
- A new architecture-to-code mapping document was introduced under `docs/architecture/repository-code-map.md`.
