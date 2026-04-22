# Bharat-OS – Scaffold, TODO, and Architecture-Separation Review
**Date:** 2026-04-22
**Branch reviewed:** `developer`
**Status:** Draft
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

The current roadmap already identifies the following as critical gaps:

- duplicate or legacy subsystem ownership,
- service runtime incompleteness,
- architecture drift,
- build system exposing non-functional or incomplete components.

This means scaffold and mixed-ownership cleanup is not cosmetic work. It is a prerequisite for making the platform trustworthy and easier to scale.

---

## 3. Review objectives

### O1. Inventory incompleteness
Find all scaffold, placeholder, no-op, permissive fallback, fake-success, and temporary compatibility implementations.

### O2. Inventory unresolved work markers
Find all `TODO`, `FIXME`, `HACK`, `XXX`, `TEMP`, and similar notes.

### O3. Identify ownership violations
Mark code that should move between `arch`, `hal`, `platform`, `kernel`, `services`, `stacks`, or `uapi`.

### O4. Identify mixed-architecture files
Mark files where multiple architecture families are implemented in one place and should be separated.

### O5. Improve build truthfulness
Ensure incomplete subsystems are either:
- clearly gated,
- explicitly marked transitional,
- or removed from default build/runtime paths.

---

## 4. Classification model

Each finding must be classified with one of the following actions:

- `IMPLEMENT_NOW`
- `MOVE_TO_CORRECT_LAYER`
- `SPLIT_BY_ARCH`
- `GATE_OFF_BY_DEFAULT`
- `DEPRECATE_AND_REMOVE`
- `KEEP_AS_SCAFFOLD_WITH_TRACKING`

Priority scale:

- `P0` = blocks correctness / security / architectural invariants
- `P1` = strong architectural cleanup needed soon
- `P2` = useful cleanup, medium urgency
- `P3` = low-risk hygiene

Status scale:

- `OPEN`
- `IN_PROGRESS`
- `DONE`
- `DEFERRED`

---

## 5. Search methodology

Repository-wide search should include at minimum:

- `TODO`
- `FIXME`
- `XXX`
- `HACK`
- `TEMP`
- `stub`
- `placeholder`
- `scaffold`
- `compat`
- `legacy`
- `transitional`
- `not implemented`
- `return 0;` or unconditional success in suspicious code paths
- `allow all` / permissive fallback patterns
- architecture switches inside shared files:
  - `#if defined(__x86_64__)`
  - `#if defined(__aarch64__)`
  - `#if defined(__arm__)`
  - `#if defined(__riscv)`
  - board/arch branching inside same `.c` file

---

## 6. Findings inventory

| ID | Area | File/Path | Finding Type | Summary | Current Owner | Correct Owner | Action | Priority | Status |
|----|------|-----------|--------------|---------|---------------|---------------|--------|----------|--------|
| F-001 | MM | `hal/hal_pt.c` | Mixed arch | Architecture-specific TLB and PT initialization mixed with weak symbols | hal | arch/hal | SPLIT_BY_ARCH | P1 | OPEN |
| F-002 | CPU | `hal/common/discovery.c` | Mixed arch | `x86_64`, `aarch64`, `riscv` CPU capabilities parsing mixed in common HAL | hal | arch/hal | SPLIT_BY_ARCH | P1 | OPEN |
| F-003 | Power | `kernel/src/power_thermal_perf.c` | Policy Creep / Mixed arch | Kernel contains topology defaults, thermal policy logic, and arch checks | kernel | services/arch | MOVE_TO_CORRECT_LAYER | P0 | OPEN |
| F-004 | Storage | `services/system/filesystem/main.c` | Mixed arch / Mock | App profile and HW arch checks directly in filesystem service main; simulated block device | services | stacks/services | KEEP_AS_SCAFFOLD_WITH_TRACKING | P1 | DONE |
| F-005 | Video | `kernel/src/display/boot_video_map.c` | Mixed arch | TLB flushes and VA calculations using arch macros directly in kernel | kernel | arch/hal | MOVE_TO_CORRECT_LAYER | P1 | DONE |
| F-006 | Drivers | `drivers/serial/ns16550/ns16550.c` | Mixed arch | Architecture-specific `x86_inb`/`x86_outb` mixed with MMIO | drivers | arch/drivers | SPLIT_BY_ARCH | P1 | OPEN |
| F-007 | Benchmark | `kernel/src/benchmark/benchmark.c` | Scaffold / Mixed arch | Fake memory usage, `clock_gettime` fallback, mixed arch `rdtsc` implementations | kernel | kernel/arch | SPLIT_BY_ARCH | P2 | OPEN |
| F-008 | CPU | `kernel/src/cpu_local.c` | Mixed arch | Arch-specific `msr tpidr_el1`, `mv tp` inline assembly in kernel | kernel | arch/kernel | SPLIT_BY_ARCH | P1 | DONE |
| F-009 | Tests | `kernel/src/tests/ktest_unmapped_fault.c` | Mixed arch | Architecture-specific memory map macros and inline assembly | kernel | arch/kernel | SPLIT_BY_ARCH | P2 | OPEN |
| F-010 | Tests | `services/faultmgr/main.c` | Scaffold | Infinite loops with `break` to avoid stub freezing; missing real IPC | services | services | KEEP_AS_SCAFFOLD_WITH_TRACKING | P2 | OPEN |
| F-011 | Build | `services/devmgr/CMakeLists.txt` | Build | Gating off devmgr components / `TODO` missing libraries | services | services | GATE_OFF_BY_DEFAULT | P1 | OPEN |

---

## 7. High-risk categories to look for

### 7.1 Mixed architecture in one source file
Typical smell:
- one `.c` file contains multiple `#ifdef` branches for unrelated arch implementations,
- per-arch constants, traps, MMU, cache, interrupt, or syscall entry details live together.

**Preferred direction**
- common contract in shared header,
- arch-specific implementation under:
  - `arch/x86/x86_64/...`
  - `arch/arm/arm32/...`
  - `arch/arm/arm64/...`
  - `arch/riscv/riscv32/...`
  - `arch/riscv/riscv64/...`

### 7.2 HAL vs arch confusion
Typical smell:
- HAL contains arch implementation instead of contract/common glue,
- arch code redefines cross-arch policy or platform behavior.

**Preferred direction**
- HAL = contract + minimal glue,
- arch = ISA implementation,
- platform = machine wiring.

### 7.3 Kernel policy creep
Typical smell:
- retry/backoff/fallback/routing/model selection/policy decisions inside `kernel/`.

**Preferred direction**
- kernel keeps deterministic mechanisms only,
- policy moves to `services/` or `stacks/`.

### 7.4 Transitional compatibility ownership
Typical smell:
- “temporary” wrapper becomes permanent surface,
- duplicated ownership between legacy and new subsystem.

**Preferred direction**
- make one primary owner,
- rename transitional surfaces clearly,
- delete or gate duplicate path.

### 7.5 Scaffold masquerading as support
Typical smell:
- code compiles,
- function exists,
- logs success,
- but runtime capability is missing.

**Preferred direction**
- either implement,
- or gate off,
- or fail explicitly and document.

---

## 8. Refactor rules

### Rule R1 — Split by responsibility, not by convenience
Do not keep mixed logic together just because it is short.

### Rule R2 — Preserve common contract, separate implementation
One interface/header is fine. Multiple architecture implementations should not live in the same body unless there is a proven reason.

### Rule R3 — Default build must not exaggerate maturity
If a subsystem is scaffold-only, do not expose it as production-ready in default paths.

### Rule R4 — Every retained scaffold must have tracking
If a scaffold remains, it must have:
- explicit comment,
- issue/review link,
- build/runtime gating decision,
- next-step owner.

### Rule R5 — Remove silent permissive behavior
Temporary permissive paths must be converted to:
- real implementation,
- explicit deny/fail,
- or guarded developer-only behavior.

---

## 9. Recommended execution order

### Phase A — Truthfulness and inventory
1. Build the inventory of scaffold/TODO/mixed-arch findings.
2. Mark P0 and P1 items.
3. Gate obviously incomplete default-build components.

### Phase B — Ownership cleanup
1. Move policy out of kernel where clear.
2. Resolve duplicate/transitional ownership.
3. Rename misleading compatibility surfaces.

### Phase C — Architecture split
1. Separate per-arch implementation files.
2. Keep one shared contract layer.
3. Add build coverage for each split path.

### Phase D — Hardening
1. Replace fake success / no-op runtime paths.
2. Add tests.
3. Update architecture docs.

---

## 10. Immediate code-agent tasks

### Task A — Repository scan
Produce a machine-generated inventory grouped by:
- scaffold/stub,
- TODO/FIXME,
- mixed arch,
- ownership drift,
- legacy/transitional duplication.

### Task B — First cleanup patch
Land a safe initial patch that:
- gates incomplete pieces,
- removes low-risk fake-success paths,
- splits at least one high-value mixed-arch file.

### Task C — Architecture split follow-up
Pick the next most harmful shared file mixing x86/arm/riscv logic and separate it.

### Task D — Docs sync
Update this review with:
- exact paths,
- action choice,
- rationale,
- progress.

---

## 11. Definition of done

This review item is only considered complete when:

- the inventory exists,
- P0/P1 findings are classified,
- at least the highest-risk mixed-architecture and scaffold issues are addressed,
- build defaults reflect real maturity,
- retained scaffolds are explicitly tracked,
- architecture ownership is cleaner than before.

---

## 12. Open tracking sections

### Completed
- Split `kernel/src/cpu_local.c` inline assembly into architecture-specific files under `arch/`.
- Replace inline-assembly architecture-specific flushes in `kernel/src/display/boot_video_map.c` with neutral HAL call `hal_tlb_invalidate_all()`.
- Gated scaffolded/experimental service `system/filesystem` behind `BHARAT_BUILD_EXPERIMENTAL_SERVICES` in `services/CMakeLists.txt`.

### In progress
-

### Deferred
-

### Removed / retired paths
-

---

## 13. Suggested follow-up review docs

- `docs/reviews/arch_platform_hal_boundary_enforcement_review_YYYY-MM-DD.md`
- `docs/reviews/default_build_truthfulness_review_YYYY-MM-DD.md`
- `docs/reviews/transitional_compatibility_surface_cleanup_review_YYYY-MM-DD.md`
- `docs/reviews/per_arch_source_split_review_YYYY-MM-DD.md`
