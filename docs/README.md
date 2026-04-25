# Bharat-OS Documentation

This directory is the canonical documentation hub for Bharat-OS.

## Start here

- **Architecture index:** [`docs/architecture/README.md`](architecture/README.md)
- **ADR index:** [`docs/adr/README.md`](adr/README.md)
- **Build/run/testing docs:** [`docs/testing/`](testing/) and root-level [`BUILD.md`](../BUILD.md)
- **Repository code map:** [`docs/architecture/repository-code-map.md`](architecture/repository-code-map.md)
- **Archived material:** [`docs/archive/`](archive/)

## Active documentation layout

### 1) Architecture (`docs/architecture/`)
Design contracts, subsystem architecture, roadmap notes, and cross-component boundaries.

Key subfolders:
- `boot/`
- `core/`
- `core/kernel/`
- `memory/`
- `storage/`
- `security/`
- `network/`
- `core/personalities/`
- `contracts/`

### 2) ADRs (`docs/adr/`)
Decision records that take precedence when roadmap or design docs disagree.

### 3) Testing (`docs/testing/`)
Execution matrix and end-to-end testing guidance.

### 4) AI agent guidance (`docs/ai-agents/`)
Standards, templates, and platform-specific agent instructions.

### 5) Supporting docs
- `docs/dev/`
- `docs/boards/`
- `docs/profiles/`
- `docs/reviews/`
- `docs/research_doc/`

### 6) Archive (`docs/archive/`)
Superseded/legacy material retained for traceability.

## Source-of-truth rule

When there is a conflict:

1. **Accepted ADRs in `docs/adr/`** win.
2. Then architecture contracts (`docs/architecture/contracts/`).
3. Then subsystem roadmap/design docs.

## Documentation quality rules

- Prefer links to real code paths (for example `core/`, `interface/`, `quality/`, `tools/`, `delivery/`).
- Use explicit status labels (`Draft`, `Active`, `Superseded`) for planning docs.
- Move stale content to `docs/archive/` instead of deleting it.
