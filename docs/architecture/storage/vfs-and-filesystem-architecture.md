# Storage and Filesystem Architecture Contract (Normative)

## 1. Purpose and scope

This document is the **normative contract** for storage/filesystem ownership and boundaries in Bharat-OS.

It defines what is **allowed**, **forbidden**, and **owned** by each layer. It does **not** describe implementation maturity or roadmap sequencing.

For implementation status, see `file-system-detailed-design.md`.
For delivery sequencing, see `roadmap.md`.
For sandbox policy details, see `sandbox-policy.md`.

---

## 2. Ownership matrix (canonical)

| Area | Primary ownership | Must do | Must not do |
|---|---|---|---|
| `drivers/block/*` | Block frontend engines | Consume normalized block requests, submit hardware-facing I/O | Path parsing, namespace logic, mount policy, POSIX permission semantics |
| `drivers/storage/*` | Transport/controller specialization | Implement protocol/controller queueing and device management | Path policy, VFS policy, capability policy decisions |
| `stacks/storage/*` | Reusable storage primitives | Provide block/cache/profile/backend adapter building blocks | Own user ABI contract or global namespace policy |
| `core/services/system/filesystem/*` | Filesystem service policy | Own namespace, mount, FD lifecycle, capability-aware access policy, request dispatch | Push policy back into kernel internals |
| `core/kernel/include/fs/*` | Shared kernel-service contract types | Define minimal shared ABI/type contracts needed by both sides | Encode namespace/mount/path policy |
| `core/kernel/src/fs/*` | Transitional mechanism-only compatibility surface | Keep minimal dispatch/forwarding/error bridging while migration completes | Introduce new pathname, namespace, mount, or sandbox policy |
| `core/lib/fs/*` | Client boundary | Provide stable client entrypoints while service IPC/runtime matures | Re-implement service policy logic |

---

## 3. Layer contract

### Layer A — Drivers (`core/drivers/block`, `core/drivers/storage`)

- Hardware/transport mechanism only.
- No policy authority for namespace, mount, descriptor, or capability semantics.

### Layer B — Storage stack (`core/stacks/storage`)

- Reusable mechanisms (block API, cache, profile resolution, backend adapters).
- May encode tunable behavior, but not user-visible policy ownership.

### Layer C — Filesystem service (`core/services/system/filesystem`)

- Sole owner of:
  - namespace visibility,
  - mount policy,
  - file descriptor lifecycle,
  - capability interpretation for filesystem access.

### Layer D — Kernel compatibility surface (`core/kernel/include/fs`, `core/kernel/src/fs`)

- Transitional surface only.
- Kernel stays mechanism-oriented and migration-safe.

### Layer E — Client boundary (`lib/fs`)

- Stable call boundary for applications/components during transport/runtime migration.
- Must remain thin and policy-neutral.

---

## 4. Kernel FS transitional allowlist (normative)

`core/kernel/src/fs/*` is temporary and constrained.

### Allowed in kernel FS transitional code

1. ABI-preserving stubs and forwarding shims.
2. Shared object/type plumbing strictly required for compatibility.
3. Minimal dispatch glue to reach service-owned implementation.
4. Error-code forwarding/translation without policy reinterpretation.
5. Validation hooks limited to structural sanity checks.

### Forbidden in kernel FS transitional code

1. Pathname policy (path prefix authorization, directory policy, path ACL logic).
2. Namespace policy (mount namespace visibility, remap rules, per-process namespace shaping).
3. Mount policy (who may mount, mount flag governance, storage-class admission).
4. Sandbox policy (`noexec/nodev/nosuid` policy ownership, sandbox defaulting, tenant policy matrices).
5. Rich capability interpretation beyond minimal validation hooks.

Any new kernel FS code must justify itself as transitional mechanism support and must not expand policy ownership.

---

## 5. Capability and authority contract

- No ambient authority assumptions.
- Filesystem authority is capability-scoped and enforced by service-owned policy.
- Service-level capability checks must gate namespace, mount, FD, and storage-class decisions.
- Kernel transitional path may validate structure, but policy decisions remain service-owned.

---

## 6. Storage classes (contract-level)

Bharat-OS storage-class policy distinguishes at least:

- filesystem mounts,
- raw block interfaces,
- blob/object backends.

Authorization for storage-class use is service-owned policy, with sandbox policy rules defined in `sandbox-policy.md`.

---

## 7. Change-control rules for this contract

A storage/filesystem PR must update this document when it changes any normative boundary involving:

- ownership movement across driver/stack/service/core/kernel/lib layers,
- kernel transitional allowlist scope,
- capability/policy ownership boundaries.

If implementation diverges from this contract, implementation must be fixed or deviation must be explicitly approved and documented in the same change.
