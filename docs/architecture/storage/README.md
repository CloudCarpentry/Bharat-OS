---
title: Storage & Filesystem Architecture (Canonical Index)
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - storage
see_also:
  - README.md
---
# Storage & Filesystem Architecture (Canonical Index)

This directory is the **canonical home** for Bharat-OS storage and filesystem architecture.

## Canonical documents

- [`vfs-and-filesystem-architecture.md`](vfs-and-filesystem-architecture.md): **normative contract** (ownership, boundaries, allowlists).
- [`file-system-detailed-design.md`](file-system-detailed-design.md): **current implementation state** (what is real/partial/transitional now).
- [`sandbox-policy.md`](sandbox-policy.md): **canonical sandbox policy** for namespace/path/storage-class controls.
- [`roadmap.md`](roadmap.md): **future delivery sequence** and platform gates.

## Reading order

1. Start with the contract.
2. Validate current state in detailed design.
3. Apply sandbox policy constraints.
4. Use roadmap for planned changes.

## Authoritative source of truth

**The current `developer` branch folder structure is authoritative.**
Older service/system/filesystem or stacks/storage references in documents are historical/proposed only unless those folders exist in the code. Storage/filesystem implementation currently follows the active repository layout.

## Consolidation note

Older architecture notes discussed overlapping topics and are now subordinate to this folder for storage/filesystem boundaries:

- `docs/architecture/vfs-storage-architecture.md`
- `docs/architecture/storage-and-sandbox-strategy.md`
- `docs/architecture/profile-driven-storage-network-subsystems.md` (storage portions)

When content conflicts, this folder is source of truth.
