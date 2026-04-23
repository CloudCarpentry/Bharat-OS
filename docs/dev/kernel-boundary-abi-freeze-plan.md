# Kernel Boundary + Syscall ABI Freeze Plan

## Purpose

Freeze a personality-neutral, profile-safe kernel boundary that remains stable across:

- kernel ↔ user transitions
- capability-mediated syscalls
- BIDL-generated service contracts
- future personality/profile expansion

This document defines what is part of the stable ABI, what is not, and how ABI drift is prevented.

---

## 1. Objectives

The ABI freeze effort exists to ensure that Bharat-OS can evolve internally without silently breaking:

- user-space binaries
- service contracts
- generated BIDL bindings
- cross-profile compatibility assumptions

The primary goals are:

1. Establish a **single canonical UAPI source of truth**
2. Freeze **syscall numbering and argument carrier layouts**
3. Separate **kernel-internal status**, **syscall errno**, and **service contract status**
4. Prevent ABI drift through **compile-time and runtime validation**
5. Enforce a strict **kernel / UAPI / user-space layering model**

---

## 2. ABI Layers

Bharat-OS has three distinct interface layers.

### 2.1 Kernel Internal Layer

Location:

- `kernel/include/`
- `kernel/src/`

Purpose:

- internal data structures
- private scheduler, MM, IPC, capability, VFS, and driver internals
- internal status/error values
- implementation details that may change freely

Rules:

- Kernel-private headers are **not ABI**
- Kernel-private types must never be exposed directly to user-space
- Kernel-private enums/structs may change without ABI version impact

---

### 2.2 UAPI Layer (Stable ABI)

Canonical location:

- `include/bharat/uapi/`

Purpose:

- syscall numbers
- syscall argument carrier structs
- stable scalar ABI types
- syscall errno values
- stable BIDL-visible contract types
- shared layout-visible structures

Rules:

- This is the **single source of truth** for all user-visible ABI
- Anything in this path is considered ABI-visible and stability-sensitive
- Kernel and user-space must both consume these headers directly
- UAPI headers must not depend on kernel-private headers

---

### 2.3 User-Space Layer

Location:

- `user/`
- `services/`
- generated BIDL bindings
- runtime libraries consuming UAPI

Rules:

- User-space may include `include/bharat/uapi/`
- User-space must not include kernel-private headers
- User-space-visible status and struct layouts must come only from UAPI

---

## 3. Canonical UAPI Ownership

### Rule: one source of truth

All stable ABI definitions must live under:

- `include/bharat/uapi/`

This includes:

- syscall numbering tables
- `SYS_E*` / syscall errno model
- ABI scalar ID types
- syscall carrier structs
- capability-visible public IDs
- BIDL-shared layout-visible types

### Rule: no shadow UAPI

The following are prohibited:

- duplicate UAPI headers under `kernel/include/uapi/`
- wrapper headers that re-export canonical UAPI definitions
- alternate copies of `SYS_*`, syscall IDs, or ABI-visible struct definitions
- duplicate include-guard names across wrapper and canonical headers

### Rationale

Shadow headers and wrapper headers create ambiguity around ownership and can silently suppress the real definitions, which is exactly the class of bug that breaks boundary discipline.

---

## 4. Stable ABI Surface

The following are part of the ABI freeze boundary.

### 4.1 Syscall numbering

- syscall numbers are canonical and append-only
- existing assigned syscall numbers must not be renumbered
- removed syscalls must leave tombstones or reserved slots unless a formal ABI version break is approved

### 4.2 Stable scalar ID types

Stable width identifiers exposed across the boundary must use canonical UAPI typedefs, such as:

- handles
- object IDs
- capability IDs
- endpoint IDs
- service-visible contract IDs where applicable

Rules:

- width must be explicit
- signedness must be explicit
- no platform-dependent scalar types in ABI-visible fields

### 4.3 Syscall argument carrier structs

Complex syscall payloads must use struct carriers defined in UAPI.

Rules:

- append-only evolution
- field reordering is forbidden
- implicit padding must be avoided where practical
- explicit reserved fields should be used when future growth is expected

### 4.4 Syscall errno model

Syscall-facing error values must be stable and UAPI-defined.

Rules:

- syscall return paths expose `SYS_*` / `sys_errno_t`
- user-space must not observe kernel-private `K_ERR_*`
- stable meaning of existing errno values must not be changed

### 4.5 Service/BIDL status model

Service responses must use contract-level status values, not raw syscall errno.

Rules:

- BIDL-visible responses use `bharat_status_t` or an approved stable alias
- syscall errno must not leak into service contract payloads except where explicitly modeled
- generated bindings must preserve canonical status width and type identity

---

## 5. Three-Layer Error / Status Model

Bharat-OS uses three separate error spaces.

### 5.1 Kernel-internal status

Examples:

- `kstatus_t`
- `K_ERR_*`

Purpose:

- internal implementation status
- scheduler, MM, IPC, capability, VFS, driver, and HAL internals

Rules:

- kernel-only
- not ABI
- may be richer than syscall errno
- may evolve as internals evolve

### 5.2 Syscall boundary errno

Examples:

- `sys_errno_t`
- `SYS_EINVAL`, `SYS_ENOMEM`, etc.

Purpose:

- stable syscall-visible failure model

Rules:

- defined in canonical UAPI
- returned across trap/syscall boundary
- must remain stable once published

### 5.3 Service / BIDL contract status

Examples:

- `bharat_status_t`

Purpose:

- service-level, transport-neutral, contract-visible status

Rules:

- used in BIDL replies and service contracts
- not interchangeable with syscall errno
- stable width and semantics across bindings

---

## 6. Mapping Rules

### 6.1 Kernel → syscall mapping

A central mapping layer must exist from kernel-internal status to syscall errno.

Rules:

- mapping must be centralized
- syscall implementations should not invent ad-hoc errno translations
- unmapped internal failures should collapse deterministically to an approved fallback errno

Recommended implementation:

- `kernel/src/lib/status.c`
- `kstatus_to_sys_errno(...)`

### 6.2 Syscall errno must not leak into BIDL contracts

Rules:

- service replies must not directly expose `SYS_*` as contract status
- if a service internally depends on a syscall result, it must translate into `bharat_status_t` or service-defined contract semantics

### 6.3 BIDL-generated code constraints

Generated code must assert:

- status type identity
- ID width compatibility
- struct layout compatibility for carrier types
- append-only evolution rules where applicable

---

## 7. Header Inclusion Rules

### Allowed

Kernel code may include:

- kernel-private headers
- canonical UAPI headers from `include/bharat/uapi/`

User-space code may include:

- canonical UAPI headers from `include/bharat/uapi/`

### Forbidden

UAPI headers must not include:

- `kernel/include/...`
- private kernel implementation headers
- profile-specific or arch-private internal headers

### Consequence

This guarantees that UAPI stays transportable, stable, and independent from kernel implementation churn.

---

## 8. Build Isolation Rules

Kernel and user-space compilation domains must remain isolated.

### Kernel build

May define:

- `__KERNEL__=1`

Must not define:

- `__USER__=1`

### User-space build

May define:

- `__USER__=1`

Must not define:

- `__KERNEL__=1`

### Forbidden state

The following is invalid:

- both `__KERNEL__` and `__USER__` defined in the same compile unit

### Rationale

Mixed visibility corrupts boundary assumptions and can expose the wrong declarations, layouts, or constants.

---

## 9. Struct Layout and Evolution Policy

ABI-visible structs must follow strict layout rules.

### Required rules

- fixed-width integer types only for layout-visible fields
- explicit reserved fields where future growth is expected
- no field reordering after publication
- no width changes after publication
- no semantic repurposing of existing fields
- alignment-sensitive structures should be verified with compile-time assertions

### Recommended checks

- `static_assert(sizeof(... ) == expected)`
- `static_assert(offsetof(..., field) == expected)`
- host-side runtime sanity tests for layout and serialization/deserialization paths

---

## 10. Versioning Policy

ABI changes are classified as:

### 10.1 Compatible additive change

Allowed if all conditions hold:

- existing fields unchanged
- new fields appended only
- old syscall numbers preserved
- existing semantics preserved
- old binaries remain functional

### 10.2 Incompatible change

Requires explicit ABI version review and approval:

- renumbering syscalls
- changing struct field widths
- reordering ABI-visible fields
- changing stable errno semantics
- changing scalar ID widths
- removing published symbols without compatibility handling

---

## 11. CI and Drift Enforcement

ABI freeze is not real unless CI enforces it.

### Required CI gates

1. UAPI compile checks
2. ABI layout tests
3. generated BIDL compatibility checks
4. syscall argument carrier checks
5. drift detection on canonical UAPI headers

### Minimum enforcement

CI must fail if:

- canonical UAPI headers drift incompatibly
- a published struct changes size unexpectedly
- a field offset changes unexpectedly
- generated BIDL bindings use the wrong status type
- a kernel-private header becomes part of UAPI transitively

To enforce this, we have developed `tools/abi/generate_abi_manifests.py` and run it automatically in GitHub Actions via `.github/workflows/abi-compat.yml`. This system splits the guardrails into four independent gates:

1. **Syscall table policy**: Checks `syscall_table.def` to ensure numbers are append-only.
2. **Carrier layout compatibility**: Parses UAPI headers (using `pycparser`) to enforce struct fields remain append-only and do not change types.
3. **IDL/BIDL compatibility**: Parses `.bidl` files to track RPC signatures, enum constants, and struct layouts.
4. **SDK symbol compatibility**: Tracks exported symbols using `nm`.

### How to update baselines

Intentional boundary changes require an explicit update process. Only approved PRs should include baseline updates. Developers can run the script with the `--update` flag:

```bash
python3 tools/abi/generate_abi_manifests.py --update
```

This will update the `.json` manifest files inside the `contracts/abi/` directory. CI will run the script in `--check` mode.

---

## 12. Immediate Repository Rules

The following repository-level rules are now in force.

### Rule A

Canonical user-visible ABI definitions belong only in:

- `include/bharat/uapi/`

### Rule B

`kernel/include/uapi/` must not contain shadow copies or wrapper re-export headers for canonical UAPI.

### Rule C

Any remaining ad-hoc syscall payloads must be migrated to canonical UAPI carrier structs.

### Rule D

Generated BIDL code must validate:

- stable status type
- stable ID widths
- stable contract-visible field layouts

### Rule E

Kernel-private status values must be translated before crossing the syscall boundary.

---

## 13. Immediate Execution Backlog

### Phase 1 — boundary cleanup

1. Remove or collapse shadow/wrapper UAPI headers
2. Ensure kernel includes canonical UAPI directly
3. fix build isolation so kernel objects never compile with both `__KERNEL__` and `__USER__`

### Phase 2 — error/status enforcement

4. centralize `kstatus_t` → `sys_errno_t` mapping
5. audit syscalls for ad-hoc errno returns
6. audit services/BIDL replies for raw syscall errno leakage

### Phase 3 — ABI validation

7. add compile-time layout assertions for stable UAPI structs
8. add runtime host ABI sanity tests
9. gate CI on UAPI drift and BIDL compatibility checks

### Phase 4 — freeze discipline

10. enforce append-only growth for syscall carriers
11. document ABI version bump policy
12. require explicit review for any incompatible UAPI change

---

## 16. Current implementation status (March 2026)

This section tracks what is landed in-tree versus what remains open.

### 16.1 Landed

- Canonical mapping APIs are present:
  - `kstatus_to_sys_errno(kstatus_t)`
  - `kstatus_to_sysret(kstatus_t)`
- Trap/syscall dispatch now uses canonical `SYS_E*` constants for trap-local invalid/perm/nosys returns.
- Trap/syscall dispatch has a shared normalization path so mixed internal return domains
  (canonical syscall errno, rich `kstatus_t`, and legacy subsystem-local negatives)
  are collapsed into stable syscall return encoding.

### 16.2 In progress

- Legacy kernel subsystems still return heterogeneous negative values (`-1`, `-2`, custom enums)
  instead of pure `kstatus_t` or canonical syscall errno.
- Current normalization includes deterministic fallbacks per syscall family while migration is ongoing.

### 16.3 Open work (next)

1. Convert syscall-facing kernel subsystems to return `kstatus_t` (or explicit canonical syscall errno)
   instead of ad-hoc negative values.
2. Add syscall-path conformance checks ensuring trap exit values are always:
   - `0`, or
   - `-(sys_errno_t)` where errno is from canonical UAPI.
3. Add service/BIDL CI checks proving `bharat_status_t` is used for contract-level status
   and raw `SYS_*` does not leak into contract payload status fields.
4. Add CI gates for ABI freeze:
   - syscall carrier/layout assertions
   - UAPI drift checks
   - generated binding type-width checks

### 16.4 Updated phase status

- **Phase 1 (boundary cleanup):** Partial / mostly complete in include ownership and layering.
- **Phase 2 (error/status enforcement):** Partial / active migration.
- **Phase 3 (ABI validation):** Not started in CI-complete form.
- **Phase 4 (freeze discipline):** Not started (policy hooks pending hard enforcement).

---

## 14. Definition of Done

This freeze plan is considered operational when:

- canonical UAPI is the only user-visible ABI source
- syscall numbering is stable and enforced
- error/status layering is implemented and audited
- wrapper/shadow UAPI patterns are removed
- build isolation is clean
- ABI drift tests run in CI
- generated BIDL bindings are type-checked against canonical UAPI

---

## 15. Summary

A stable kernel boundary requires:

- one canonical UAPI
- strict layering
- isolated build domains
- centralized status translation
- append-only syscall carriers
- CI-backed ABI drift enforcement

This is the line between “headers that currently compile” and a real OS ABI that can survive growth.

---

## 17. Companion policy: native standard surface vs compatibility

The syscall ABI freeze defined in this document protects low-level kernel/user binary compatibility.
To avoid architecture drift at the SDK/runtime layer, Bharat-OS also maintains a separate policy for
what belongs to native Bharat contracts versus personality compatibility APIs.

See:

- `docs/dev/bharat-native-standard-surface-and-compat-boundary.md`

Use both documents together when proposing new public headers:

- This document answers: **is it stable/ABI-safe and correctly layered?**
- The companion policy answers: **is it Bharat-native or compatibility-only?**
