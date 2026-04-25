# Architecture Documentation Index

This index maps Bharat-OS architecture documentation to the current repository structure.

## Recommended reading order

1. [`folder_structure.md`](folder_structure.md) – repository boundaries and structural intent.
2. [`repository-code-map.md`](repository-code-map.md) – architecture docs mapped to live code folders.
3. [`core/README.md`](core/README.md) – core runtime/control-plane architecture.
4. [`boot/BOOT_ARCHITECTURE.md`](boot/BOOT_ARCHITECTURE.md) – boot contracts and bring-up shape.
5. [`kernel-object-model.md`](kernel-object-model.md), then the detailed kernel folders:
   - [`kernel/scheduler/README.md`](kernel/scheduler/README.md)
   - [`kernel/tasks-threads/README.md`](kernel/tasks-threads/README.md)
   - [`kernel/ipc/README.md`](kernel/ipc/README.md)
   - [`kernel/urpc/README.md`](kernel/urpc/README.md)
6. [`memory/memory-architecture-comprehensive.md`](memory/memory-architecture-comprehensive.md)
7. [`storage/README.md`](storage/README.md)
8. [`security/crypto/overview.md`](security/crypto/overview.md)
9. [`contracts/`](contracts/) and ADRs in [`../adr/`](../adr/)

## Current architecture areas

- **Boot:** `boot/`
- **Core runtime model:** `core/`
- **Kernel internals:** `kernel/`, `interrupt/`
- **Memory:** `memory/`
- **Storage:** `storage/`
- **Security & crypto:** `security/`
- **Network:** `network/`
- **Personalities & compat layers:** `personalities/`
- **Formal contracts/specs:** `contracts/`

## Documentation cleanup status

As part of the April 2026 consolidation:

- Archived legacy/backup docs were moved under `docs/archive/`.
- Broken links to removed or renamed files were eliminated from this index.
- A repository/code mapping document was added to reduce architecture-vs-code drift.

## Conflict resolution

If two docs disagree, follow this order:

1. Accepted ADR in `docs/adr/`
2. Contract docs in `docs/architecture/contracts/`
3. Subsystem architecture docs
4. Roadmap/proposal docs
