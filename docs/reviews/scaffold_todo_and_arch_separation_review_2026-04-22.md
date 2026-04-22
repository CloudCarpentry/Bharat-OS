# Bharat-OS – Scaffold, TODO, and Architecture-Separation Review
**Date:** 2026-04-22  
**Branch reviewed:** `developer`  
**Status:** Draft (First-pass inventory + initial cleanup landed)  
**Owner:** Architecture / Core Maintainers

---

## 1. Purpose

This review tracks three related cleanup problems in the Bharat-OS codebase:

1. scaffold / placeholder / stubbed implementations that create an illusion of completeness,
2. unresolved `TODO` / `FIXME` / temporary compatibility paths,
3. source files that mix architecture-specific logic (`x86_64`, `arm*`, `riscv*`) or violate intended ownership boundaries.

The goal is to reduce architecture drift and improve code truthfulness, maintainability, and production readiness.

This review follows the Bharat-OS architectural boundary model:

- `arch/` = ISA / CPU-specific implementation
- `hal/` = abstraction contracts and common glue
- `platform/` = board / SoC / machine integration
- `kernel/` = mechanism only
- `services/` = policy / orchestration
- `stacks/` = composed subsystem stacks

---

## 2. Why this matters

Current roadmap and code status indicate major risks from:

- duplicate/transitional subsystem ownership,
- service runtime incompleteness,
- architecture drift,
- build defaults exposing scaffolded paths as production-like.

This cleanup is prerequisite hardening work, not cosmetic refactoring.

---

## 3. Review objectives

### O1. Inventory incompleteness
Find scaffold, placeholder, no-op, permissive fallback, fake-success, and temporary compatibility implementations.

### O2. Inventory unresolved work markers
Find `TODO`, `FIXME`, `HACK`, `XXX`, `TEMP`, and related markers.

### O3. Identify ownership violations
Mark code that should move between `arch`, `hal`, `platform`, `kernel`, `services`, `stacks`, or `uapi`.

### O4. Identify mixed-architecture files
Mark files where multiple architecture families are implemented in one place and should be separated.

### O5. Improve build truthfulness
Ensure incomplete subsystems are clearly gated, explicitly transitional, or removed from default build/runtime paths.

---

## 4. Classification model

Actions used in this review:

- `IMPLEMENT_NOW`
- `MOVE_TO_CORRECT_LAYER`
- `SPLIT_BY_ARCH`
- `GATE_OFF_BY_DEFAULT`
- `DEPRECATE_AND_REMOVE`
- `KEEP_AS_SCAFFOLD_WITH_TRACKING`

Priority scale:

- `P0` = correctness/security/architecture invariant risk
- `P1` = high-value architecture cleanup
- `P2` = medium cleanup
- `P3` = hygiene

Status scale:

- `OPEN`
- `IN_PROGRESS`
- `DONE`
- `DEFERRED`

---

## 5. Search methodology

Repository scan commands used:

- `rg -i -n "TODO|FIXME|XXX|HACK|TEMP|stub|placeholder|scaffold|compat|legacy|transitional|not implemented"`
- `rg -l -i "TODO|FIXME|XXX|HACK|TEMP" | wc -l`
- `rg -l -i "stub|placeholder|scaffold|not implemented" | wc -l`
- `rg -l "__x86_64__|__aarch64__|__arm__|__riscv" --glob '*.[ch]' | wc -l`
- `rg -l -i "compat|legacy|transitional" | wc -l`

First-pass aggregate counts from this scan:

- TODO/FIXME/HACK/TEMP-bearing files: **208**
- scaffold/stub/placeholder files: **364**
- C/C mixed-arch macro files: **32**
- compat/legacy/transitional files: **298**

---

## 6. Findings inventory

| ID | Area | File/Path | Finding Type | Summary | Current Owner | Correct Owner | Action | Priority | Status |
|----|------|-----------|--------------|---------|---------------|---------------|--------|----------|--------|
| F-001 | Services/build | `services/CMakeLists.txt` | Build truthfulness | `system/filesystem` built by default though implementation remains scaffold-heavy | services | services | GATE_OFF_BY_DEFAULT | P0 | DONE |
| F-002 | Storage stack | `stacks/storage/block/block.c` | Fake success | Unknown device path returned success and stub info, masking unsupported runtime path | stacks/storage | stacks/storage | IMPLEMENT_NOW | P0 | DONE |
| F-003 | Service architecture split | `services/system/filesystem/main.c` | Mixed arch | Single service source carried x86/arm/riscv arch-selection preprocessor branches | services/system | services/system | SPLIT_BY_ARCH | P1 | DONE |
| F-004 | HAL common routing | `hal/hal_pt.c` | Mixed arch | Cross-arch dispatch in common file; acceptable as contract router, but needs follow-up for cleaner registration model | hal | hal+arch | KEEP_AS_SCAFFOLD_WITH_TRACKING | P2 | OPEN |
| F-005 | Service lifecycle scaffolds | `services/core/init/init_manifest.c` | Scaffold | Multiple services use `stub_start` execution path | services/core | services/core | KEEP_AS_SCAFFOLD_WITH_TRACKING | P1 | OPEN |
| F-006 | Legacy network ownership | `services/CMakeLists.txt` + `services/legacy/net` | Transitional duplication | Legacy monolithic network service coexists with netmgr/netstack forward path | services/legacy + services/net* | services/net* | DEPRECATE_AND_REMOVE | P1 | OPEN |
| F-007 | Kernel compatibility include debt | `kernel/include/**` (multiple) | TODO debt | repeated include-order compatibility TODOs in public headers | kernel/include | kernel/include | MOVE_TO_CORRECT_LAYER | P2 | OPEN |
| F-008 | Boot adapter scaffolds | `boot/src/adapters/uefi/uefi_adapter.c` | Scaffold | compile-safe placeholder returns unsupported behavior | boot | boot/platform | KEEP_AS_SCAFFOLD_WITH_TRACKING | P2 | OPEN |
| F-009 | Service stubs enabled by path | `services/system/filesystem/*` | Scaffold runtime | VFS flow still stub-driven (`vfs_stub.c`) and demo request path | services/system | services/system + stacks/storage | IMPLEMENT_NOW | P1 | IN_PROGRESS |
| F-010 | Compatibility wrappers | `tools/build/legacy_adapter.py`, wrappers | Transitional path | Legacy CLI/build path still actively supported | tools/build | tools/build | KEEP_AS_SCAFFOLD_WITH_TRACKING | P2 | OPEN |

---

## 7. High-risk categories and current status

### 7.1 Mixed architecture in one source file
- **Addressed in first pass:** `services/system/filesystem/main.c` split into per-arch files.
- **Remaining examples:** routing files in HAL and syscall stubs still contain macro dispatch.

### 7.2 HAL vs arch confusion
- `hal/hal_pt.c` remains a central router. Acceptable currently, but should move to registration-driven init in follow-up.

### 7.3 Kernel policy creep
- Initial scan found policy-like compatibility debt in kernel headers; no risky kernel behavior rewrite in this pass.

### 7.4 Transitional compatibility ownership
- Legacy network and several wrappers remain as tracked transitional surfaces.

### 7.5 Scaffold masquerading as support
- Block stack unknown-device fake success removed in this pass.

---

## 8. Refactor rules applied in this patch

- **R1:** Split by responsibility (service arch-selection moved out of shared body).
- **R2:** Preserve common contract (`fs_select_arch()` contract retained; per-arch implementations split).
- **R3:** Default build truthfulness improved (`system/filesystem` now opt-in).
- **R5:** Silent permissive behavior reduced (unknown storage device paths now fail explicitly).

---

## 9. Recommended execution order (next passes)

### Phase A — Truthfulness and inventory
1. Expand this table with machine-generated per-file index for all P0/P1 findings.
2. Gate additional scaffold-only services currently exposed by default.

### Phase B — Ownership cleanup
1. Resolve legacy net vs netmgr/netstack ownership.
2. Remove policy from mechanism-layer code paths where found.

### Phase C — Architecture split
1. Evaluate `hal/hal_pt.c` and `lib/syscall/syscall_stubs.c` for cleaner arch registration/split models.
2. Ensure split paths are covered in CI build matrix.

### Phase D — Hardening
1. Replace remaining fake-success/no-op paths in storage and init orchestration.
2. Add explicit negative-path tests for unsupported paths.

---

## 10. Immediate code-agent tasks completed in this pass

- **Task A (scan):** Completed first-pass repository scan with aggregate counts and prioritized findings.
- **Task B (safe first patch):** Completed.
  - Gated scaffold filesystem service off by default.
  - Removed low-risk fake-success behavior in block stack unknown-device path.
  - Split one high-value mixed-arch source file into per-arch files.
- **Task C (architecture split follow-up):** Deferred to next patch set for additional files.
- **Task D (docs sync):** This review document updated with inventory and actions.

---

## 11. Definition-of-done progress

- Inventory exists: **Yes (first-pass, aggregate + prioritized findings).**
- P0/P1 findings classified: **Yes (table above).**
- Highest-risk issues addressed: **Partially (initial representative patch landed).**
- Build defaults reflect maturity better: **Improved (filesystem scaffold now opt-in).**
- Retained scaffolds tracked: **Partially (table + status markers).**
- Architecture ownership cleaner: **Improved in filesystem service path.**

---

## 12. Open tracking sections

### Completed
- F-001, F-002, F-003.

### In progress
- F-009.

### Deferred
- F-004, F-006, F-007, F-008, F-010.

### Removed / retired paths
- None removed in this pass.

---

## 13. Suggested follow-up review docs

- `docs/reviews/arch_platform_hal_boundary_enforcement_review_YYYY-MM-DD.md`
- `docs/reviews/default_build_truthfulness_review_YYYY-MM-DD.md`
- `docs/reviews/transitional_compatibility_surface_cleanup_review_YYYY-MM-DD.md`
- `docs/reviews/per_arch_source_split_review_YYYY-MM-DD.md`
