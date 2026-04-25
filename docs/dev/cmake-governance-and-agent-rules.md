---
title: CMake Governance, Versioning, and Agent/Developer Rules
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - dev
see_also:
  - README.md
---
# CMake Governance, Versioning, and Agent/Developer Rules

This document defines mandatory build-system governance for Bharat-OS contributors (human and code agents).

## 1) CMake structure rules

## 1.1 Directory layout contract

- Root `CMakeLists.txt` owns global options, profile toggles, and top-level composition.
- Subsystem `CMakeLists.txt` files (e.g., `core/services/`, `core/drivers/`, `subsys/`, `lib/`) own only local target wiring.
- New components must be introduced through the nearest local `CMakeLists.txt` first, then promoted to top-level composition only when required.

## 1.2 Target hygiene

- Prefer small targets with clear ownership (`service`, `driver`, `lib`, `test`).
- Keep public headers in explicit include roots and avoid accidental include leakage.
- Explicitly declare inter-target dependencies; do not rely on transitive link side effects.
- Use option gates for experimental components (`BHARAT_BUILD_*`) so default builds remain stable.
- Driver/service/subsystem inclusion must be controlled through component-policy cache options
  (for example `BHARAT_ENABLE_DRIVER_*`, `BHARAT_ENABLE_SERVICE_*`, `BHARAT_ENABLE_SUBSYS_*`).

## 1.3 Component-policy contract

- `delivery/cmake/modules/BharatComponentPolicy.cmake` is the canonical single source of truth for profile/personality/board
  driven component requirements (legacy `cmake/modules/BharatComponentPolicy.cmake` remains as a compatibility symlink during migration).
- All top-level configuration entry points (presets, wrapper scripts, CI jobs, and agents) must pass:
  - `BHARAT_DEVICE_PROFILE`
  - `BHARAT_PERSONALITY_PROFILE`
  - `BHARAT_TARGET_BOARD`
- `BHARAT_DEVICE_PROFILE` and `BHARAT_PERSONALITY_PROFILE` are canonicalized to uppercase during configure.
- Required-component checks must fail-fast at configure time (no deferred runtime surprises).

## 1.4 Preset-first workflow

- Contributors must use `CMakePresets.json` as the primary configuration entry point.
- New target families should be reachable through at least one preset.
- If a preset is changed, include a note describing impact on host builds and cross-arch builds.

---

## 2) Versioning and compatibility rules

## 2.1 Component version metadata

- Components should expose version/build metadata through existing component registration patterns where available.
- API/ABI-impacting changes require version/interface updates and changelog notes in docs.

## 2.2 Build option compatibility

- Changes to `BHARAT_BUILD_*` options must preserve backwards compatibility where possible.
- If compatibility cannot be preserved, document migration notes in the same change.

## 2.3 Documentation-date discipline

- Status docs should carry an explicit date/snapshot context.
- Do not promote maturity labels without evidence in code/tests.

---

## 3) Agent and developer execution rules

These rules apply equally to human contributors and automated code agents.

1. **Small but solid**: ship bounded slices that compile and are testable.
2. **No silent target drift**: every new executable/library must be intentionally wired in CMake.
3. **Evidence-backed status**: if code is scaffold/partial, do not label as baseline/production.
4. **Parallel-safe changes**: keep interfaces stable; if changed, document expected downstream updates.
5. **Architecture parity awareness**: when adding kernel-facing build logic, consider x86_64/arm64/riscv64 implications.
6. **Agent parity**: automation agents (Codex, Jules, CI bots, and similar) must follow the same
   component-policy and cache-variable contract as human contributors.

---

## 4) Required checklist for CMake-affecting PRs

- [ ] `CMakePresets.json` impact reviewed.
- [ ] All edited `CMakeLists.txt` files remain option-gated where appropriate.
- [ ] Affected targets build under at least one host/dev preset.
- [ ] Documentation updated (`README.md`, `ROADMAP.md`, and/or `docs/current-code-status.md`) when maturity/status changed.
- [ ] New target maturity classified using: Scaffold / Partial / Baseline / Production.

---

## 5) Relationship to architecture docs

Architecture documents may describe forward-looking state. This is allowed and encouraged.

However, any immediate implementation claim must align with:

- `docs/current-code-status.md` for present-state maturity,
- `ROADMAP.md` for closure plan and discrepancy tracking.
