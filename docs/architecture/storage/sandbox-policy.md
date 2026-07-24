---
title: Storage Sandbox Policy (Canonical)
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
# Storage Sandbox Policy (Canonical)

## 1. Purpose

This document defines canonical storage/filesystem sandbox policy for Bharat-OS.

It complements the normative storage architecture contract and centralizes policy that was previously scattered in strategy notes.

---

## 2. Policy dimensions

Every sandbox context is evaluated across three dimensions:

1. **Namespace view**
   - which mount roots are visible.
2. **Path rights**
   - per-prefix rights: `read`, `write`, `exec`, `create`, `delete`, `metadata`.
3. **Storage-class rights**
   - whether `filesystem`, `block`, and `blob` classes are allowed.

---

## 3. Path-right matrix model

Policy evaluation should follow this order:

1. resolve caller sandbox context,
2. resolve namespace visibility for target path,
3. resolve path-prefix rights,
4. resolve storage-class permission,
5. enforce mount flags and descriptor provenance constraints.

Deny by default when any step is unresolved.

---

## 4. Storage-class policy baseline

### Filesystem class

- allowed when sandbox grants filesystem access for the namespace/path.

### Raw block class

- denied by default.
- explicit grant required for any raw block access.

### Blob/object class

- read may be granted independently of write.
- write must be explicitly scoped to approved buckets/prefixes.

---

## 5. Mount token concept

Mount operations should be capability-anchored with structured mount tokens carrying at minimum:

- allowed namespace/path scope,
- storage-class constraints,
- allowed mount flags,
- optional TTL/expiry and issuer identity.

Tokens are evaluated by filesystem service policy before mount admission.

---

## 6. Descriptor provenance

File descriptors should carry provenance sufficient to verify:

- namespace source,
- mount identity/policy context,
- storage-class lineage,
- rights envelope at open time.

Policy checks for sensitive operations must consider descriptor provenance, not only pathname re-resolution.

---

## 7. Recommended default restrictions

For writable app-facing mounts, default to restrictive baseline:

- `noexec`
- `nodev`
- `nosuid`

Additional hardening defaults:

- deny raw block unless explicitly needed,
- default blob writes to deny,
- allow only minimal required path prefixes.

---

## 8. Relationship to roadmap phases

- Phase 1: baseline capability-aware path checks and service-owned enforcement.
- Phase 2: stronger mount admission and persistent backend policy integration.
- Phase 3: full blob/object policy semantics with staged-write/commit governance.

Roadmap sequencing and delivery gates live in `roadmap.md`.
