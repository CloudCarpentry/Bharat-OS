# Project Structure Refactor Plan (Incremental, No Big-Bang)

## Objective

Migrate to a zone-based repository structure in controlled phases so that:

1. builds/tests/runs keep working at every phase,
2. documentation stays aligned with code reality,
3. tooling can evolve without blocking code migration.

This plan is based on the current repo layout and build system behavior (`build.sh` -> `tools/build.py` with target YAMLs under `tools/targets/qemu`).

---

## Migration tracker (live)

| ID | Slice | Status | Notes |
| --- | --- | --- | --- |
| A | QEMU YAML move to `delivery/targets/qemu` | ✅ Completed | Alias translation exists in `tools/build/path_aliases.py` and resolver wiring. |
| B1 | Shared alias helper adoption for target matrix + loader | ✅ Completed | `tools/targets/loader.py` now uses shared alias helper/fallback primitives. |
| B2 | Alias helper adoption for ABI tooling path resolution | ✅ Completed | `tools/abi/{check_idl_compat.py,check_struct_layouts.py,generate_abi_manifests.py}` now resolve `interface/*` canonical paths via shared `tools/build/path_aliases.py` helpers with legacy fallback warnings. |
| B3 | Guard escalation to strict mode | ✅ Completed | `kernel-ci` now runs `tools/ci/check_migration_refs.py --strict`; guard patterns cover completed A/C/B2 legacy roots. |
| B4 | Delivery config/assets root relocation + legacy symlink trimming | ✅ Completed | `configs/` and `assets/` now canonicalize to `delivery/{configs,assets}` with compatibility symlinks; obsolete `quality/*` compatibility symlink fanout removed. |
| B5 | Delivery CMake root relocation (`cmake/` -> `delivery/cmake/`) | ✅ Completed | Canonical CMake modules/toolchains now live under `delivery/cmake`; legacy top-level `cmake` is retained as a compatibility symlink while callers migrate. |
| C1 | Interface `idl/` move | ✅ Completed | `interface/idl/` is now authoritative; legacy `idl` path is preserved as a compatibility symlink and tooling fallback. |
| C2 | Interface `uapi/` move | ✅ Completed | `interface/uapi/` is now authoritative; legacy `uapi` path is preserved as a compatibility symlink. |
| C3 | Interface `sdk/` move | ✅ Completed | `interface/sdk/` is now authoritative; legacy `sdk` path is preserved as a compatibility symlink. |
| D1 | `boot/` to `core/boot/` | 🚧 In progress | D1a + D1b completed: sources/common/include/discovery/protocols are now canonical in `core/boot/*`; D1d completed: legacy `boot/{src,common,include,discovery,protocols}` are now compatibility symlinks to canonical `core/boot/*`. D1c completed: kernel sub-target CMake include wiring now uses migration-aware `BHARAT_BOOT_INCLUDE_DIR` instead of hardcoded `boot/include` paths. |
| D2 | `kernel/` to `core/kernel/` | 🚧 In progress | D2a completed: `kernel/src/{core,init,boot}` migrated to `core/kernel/src/*`; D2b completed: `kernel/include` moved to `core/kernel/include`; D2c completed: remaining `kernel/src/*` moved into `core/kernel/src/*` with legacy symlink wrappers retained at `kernel/src/*`; D2d completed: `kernel/CMakeLists.txt` now resolves `arch/hal/platform/lib` through canonical `core/*` roots with legacy fallback warnings; D2e completed: `kernel/staging/*` moved to canonical `core/kernel/staging/*` and legacy `kernel/staging` is now a compatibility symlink. |
| D3 | `arch/` + `hal/` + `platform/` to `core/*` | ✅ Completed | D3a completed: `arch/` moved to canonical `core/arch/` with legacy `arch` symlink compatibility and top-level CMake wiring updated to prefer `core/arch`. D3b completed: `hal/` moved to canonical `core/hal/` with legacy `hal` symlink compatibility and top-level CMake wiring updated to prefer `core/hal`. D3c completed: `platform/` moved to canonical `core/platform` with legacy `platform` symlink compatibility and top-level CMake wiring updated to prefer `core/platform`. |
| D4 | `lib/` + `stacks/` + `drivers/` + `services/` + `personalities/` to `core/*` (bounded slice) | ✅ Completed | D4a completed: `lib/` and `stacks` moved to canonical `core/lib` and `core/stacks`; legacy `lib` and `stacks` symlinks retained for compatibility. D4b completed: `drivers/`, `services/`, and `personalities/` moved to canonical `core/*` locations with legacy symlink compatibility retained, and top-level CMake wiring updated to prefer `core/*` roots. |
| E1 | `tests/` to `quality/tests/` (bounded Phase 5 slice) | ✅ Completed | Canonical host/unit/e2e test tree now lives under `quality/tests`; legacy `tests` path is retained as a compatibility symlink while docs and scripts transition. |
| F1 | Public umbrella `include/` to `interface/include/` | ✅ Completed | Canonical public header tree now resides in `interface/include`; legacy `include` is retained as a compatibility symlink and alias helper support was added. |
| F2 | `user/` apps+SDK to `experience/user/` | ✅ Completed | Canonical user-space tree now resides in `experience/user`; legacy `user` is retained as a compatibility symlink and top-level CMake prefers canonical path with fallback warning. |

---

## Current-state constraints observed from code/docs

- `./build.sh` is a thin wrapper that delegates to `python3 tools/build.py`, so migration must preserve script entry points even if internal paths change.  
- Build/run presets and target model are already centralized around `tools/build.py` and YAML targets in `tools/targets/qemu/`, including Linux/Android headless variants.  
- Multi-arch run support is documented around QEMU binaries for x86_64, arm64, riscv64, arm32, and riscv32 paths.

Implication: safest strategy is **compatibility-first path translation**, then progressive folder moves.

---

## Target structure (unchanged goal)

```text
Bharat-OS/
  core/
    arch/
    hal/
    kernel/
    boot/
    platform/
    drivers/
    services/
    stacks/
    personalities/
    lib/

  interface/
    uapi/
    idl/
    sdk/
    contracts/
    include/

  delivery/
    tools/
    cmake/
    configs/
    targets/
    release/
    assets/

  quality/
    tests/

  experience/
    user/

  docs/
  .github/
```

---

## Working rules for every phase (must follow)

### A) Safe migration rule

- Move only one bounded slice at a time.
- Keep old paths functional via compatibility shims/aliases until the phase closes.
- If something breaks, fix in the same phase before merge.

### B) Documentation rule

For each phase PR, update at least:

- relevant architecture/build docs under `docs/`,
- developer build/run instructions,
- any changed command examples (old + new during transition).

### C) Validation rule

Run phase validation commands (build/package/run/all) and capture outcomes in phase notes/PR.

### D) Tooling-change rule

If tooling must change in a phase (expected), do it in this order:

1. Add backward-compatible behavior.
2. Migrate callers/docs.
3. Remove deprecated path/flags in a later cleanup phase.

---

## Phase-by-phase execution plan

## Phase 0 — Baseline Freeze + Guardrails (Sprint 1)

**Goal:** Prepare repo/CI for safe movement without changing structure yet.

**Code tasks**

- Add a migration tracking doc/table with phase IDs, owners, and status.
- Add CI check that rejects creation of new top-level roots outside approved zones.
- Add CI check to detect hardcoded soon-to-be-legacy root paths in changed files.

**Tooling tasks**

- Add a path-alias utility layer in build tooling (logical path -> actual path).
- Add warning mechanism for deprecated paths (warning-only in this phase).

**Docs tasks**

- Update build docs to clearly state transitional behavior.
- Add migration policy page in `docs/dev/` linking this plan.

**Exit criteria**

- CI guardrails active.
- No functional path changes yet.
- Team can merge regular work with guardrails on.

---

## Phase 1 — Zone Scaffolding (Sprint 2)

**Goal:** Create destination zones and introduce compatibility indirection.

**Code tasks**

- Create empty/top-level zone folders: `core/`, `interface/`, `delivery/`, `quality/`.
- Add build-system include/search indirection so old and new locations resolve.
- Keep all existing paths authoritative for now; new zones are scaffolding.

**Tooling tasks**

- Build scripts resolve through alias map first, direct path second.
- Add logging that reports which path was used (old/new).

**Docs tasks**

- Document alias behavior and expected deprecation timeline.

**Exit criteria**

- Zero behavior change for contributors.
- All commands still work from legacy paths.

---

## Phase 2 — Low-risk Delivery/Interface Moves (Sprints 3–4)

**Goal:** Move low-coupling directories first.

**Move scope**

- `release/` -> `delivery/release/`
- `targets/` -> `delivery/targets/`
- `contracts/` -> `interface/contracts/`
- selected `sdk/` modules -> `interface/sdk/` (stable modules first)

**Code tasks**

- Physically move one subtree per PR (small PRs).
- Add compatibility links/mappings for moved paths.
- Update import/include/script references incrementally.

**Tooling tasks**

- Update tooling path constants to prefer new locations.
- Keep fallback to old locations.

**Docs tasks**

- Update references in README/build docs where paths changed.

**Exit criteria**

- All moved folders accessed via new paths.
- Legacy fallback still present and tested.

---

## Phase 3 — Core Runtime Migration (Sprints 5–8)

**Goal:** Move mechanism + policy trees under `core/` without breaking runtime.

**Move scope (recommended order)**

1. `boot/` -> `core/boot/`
2. `kernel/` -> `core/kernel/`
3. `hal/`, `arch/`, `platform/` -> `core/*`
4. `drivers/`, `services/`, `stacks/`, `personalities/`, `lib/` -> `core/*`

**Why this order**

- `boot`/`kernel` are high-impact; migrate with maximum visibility first.
- HAL/arch/platform then follow with boundary cleanup.
- service/policy trees move once core path plumbing is stable.

**Code tasks**

- Migrate includes and CMake path wiring in atomic slices.
- Keep adapter headers/modules where needed to avoid import breakage.
- Fix all compile/runtime regressions in-phase.

**Tooling tasks**

- Enhance path translator to cover core tree moves.
- Emit structured deprecation report in CI logs.

**Docs tasks**

- Update architecture tree diagrams and component location docs.
- Update contributor docs for new source paths.

**Exit criteria**

- Core trees primarily served from `core/`.
- Legacy paths still work through compatibility layer.

---

## Phase 4 — Interface Consolidation + Contract Enforcement (Sprints 9–10)

**Goal:** Finalize public-facing surfaces under `interface/` and enforce contract-first discipline.

**Move scope**

- `uapi/` -> `interface/uapi/`
- `idl/` -> `interface/idl/`
- remaining `sdk/` -> `interface/sdk/`
- public umbrella headers from `include/` -> `interface/include/`

**Code/tasks**

- Add CI gate: new public APIs require `uapi/idl` linkage.
- Ensure generated or packaged interface artifacts read from new paths.

**Tooling/docs**

- Update packaging/release paths for interface artifacts.
- Update API/SDK documentation and examples.

**Exit criteria**

- All contract assets (uapi/idl/sdk/include) resolve primarily from `interface/`.
- CI detects any new direct references to deprecated roots in changed files.

---

## Near-term incremental PR sequence (recommended medium chunks)

This is the execution order to avoid a big-bang refactor while still moving fast:

1. **C3 (done): SDK root relocation**
   - Move `sdk/` -> `interface/sdk/`.
   - Keep `sdk` compatibility symlink for legacy references.
   - Validate full build matrix.
2. **D1a: Boot source move with compatibility wrappers**
   - Move `boot/src/` + `boot/common/` into `core/boot/`.
   - Keep `boot/` forwarding CMake and include wrappers.
   - Update only direct build references in the same PR.
3. **D1b: Boot include/public header move**
   - Move `boot/include/` to `core/boot/include/`.
   - Keep umbrella compatibility headers in `boot/include/boot`.
4. **D2: Kernel tree move**
   - Move `kernel/` -> `core/kernel/` with top-level CMake forwarding.
   - Keep old `kernel/` path as compatibility shim until D4.
5. **D3: HAL/arch/platform move**
   - Move one tree per PR (`hal`, then `arch`, then `platform`).
   - Add include alias paths and CI deprecation warnings.
6. **D4: Drivers/services/stacks/personalities/lib move**
   - Move low-coupling trees first (`lib`, `stacks`) then service-heavy trees.
   - Keep temporary adapters and remove only after Phase E strict mode.

- Interface assets fully centralized.
- Contract gate enabled and enforced.

---

## Phase 5 — Quality/Tests Move + Cleanup Lock (Sprints 11–12)

**Goal:** finalize remaining moves and remove temporary compatibility.

**Move scope**

- `tests/` -> `quality/tests/` (completed)
  - E1 completed: moved the full `tests/` tree into `quality/tests/` and retained `tests` as a compatibility symlink.
- `tools/`, `cmake/`, `configs/`, `assets/` -> `delivery/*` (if not already completed)

**Cleanup tasks**

- Remove alias/fallback code.
- Convert deprecated-path warnings to hard CI failures.
- Remove all legacy-path references from docs/scripts.

**Exit criteria**

- New zone structure is authoritative.
- No legacy-path usage in code, CI, or docs.

---

## Validation matrix to run in every phase

Use this baseline command suite (or updated equivalent if tooling evolves in that phase).

```bash
./build.sh build --target x86_64_desktop_headless
./build.sh package --target x86_64_desktop_headless
./build.sh run --target x86_64_desktop_headless

./build.sh all --target x86_64_desktop_headless_linux
./build.sh all --target arm64_desktop_headless_linux
./build.sh all --target riscv64_desktop_headless_linux

./build.sh all --target x86_64_desktop_headless_android
./build.sh all --target arm64_desktop_headless_android
./build.sh all --target riscv64_desktop_headless_android
```

### Recommended expansion for 5 QEMU hardware arcs

To cover all 5 practical QEMU families currently represented in docs/targets:

```bash
./build.sh all --target arm32_mmu_lite_headless
./build.sh all --target riscv32_mmu_lite_headless
```

(Along with x86_64, arm64, riscv64 already listed above.)

### If QEMU is missing, install first

Ubuntu/Debian example:

```bash
sudo apt update
sudo apt install -y qemu-system-x86 qemu-system-arm qemu-system-misc
```

---

## PR slicing strategy (important)

- Prefer **one directory move per PR** + mandatory fixes.
- Keep PRs below a manageable diff size to reduce merge conflicts.
- Use branch naming like: `refactor/phase-3-boot-core-move`.
- Each PR must include:
  - move summary,
  - compatibility updates,
  - doc updates,
  - validation command results.

---

## Tooling evolution policy (because commands may change)

When build/run commands are updated in future phases:

1. Keep old commands functional for at least one phase via compatibility parser.
2. Print migration hint in command output.
3. Update all docs/examples in same PR.
4. Flip old command support from warning -> error only in cleanup phase.

---

## Risks and mitigations

| Risk | Impact | Mitigation |
| --- | --- | --- |
| Include/import path breakage | High | Compatibility include dirs + adapter headers + phase-specific compile checks |
| Runtime break after path move | High | Mandatory `build/package/run/all` matrix before merge |
| Tooling drift during refactor | High | Alias resolver + dual-path support + explicit deprecation policy |
| Docs become stale | Medium | “No code move without doc update” phase gate |
| Cross-team merge conflicts | Medium | Small PR batches + short freeze windows around large moves |

---

## Definition of done

- Zone-oriented root structure fully adopted.
- Legacy paths removed from code/tooling/docs.
- CI enforces new structure and interface contract discipline.
- Build/test/run matrix passes on supported QEMU targets.

---

## Immediate next actions (updated to current repository state)

1. Record current migration status in a tracker doc and keep it updated per PR.
2. Complete Phase B tooling unification (shared alias helper + CI reference guard).
3. Continue delivery tooling migration (`tools/`) using thin-slice PRs with mandatory validation matrix; `cmake/`, `configs/`, and `assets/` now canonicalize under `delivery/` with compatibility shims.

---

## Repo-informed execution backlog (practical medium chunks)

This section converts the phase model into concrete, code-aware slices from the current tree.

### Phase A (completed) — QEMU target YAML relocation (medium chunk, low runtime risk)

**Objective**
- Relocate QEMU target YAMLs from `tools/targets/qemu/` to `delivery/targets/qemu/` with compatibility path translation.

**Why first**
- Build orchestration is already centralized in `tools/build.py`.
- YAML target resolution is the least invasive place to add old->new aliasing before larger tree movement.

**Code changes**
- Move all `tools/targets/qemu/*.yaml` to `delivery/targets/qemu/`.
- Keep old CLI usage functional by translating `--target-yaml tools/targets/qemu/...` to `delivery/targets/qemu/...` in resolver logic.
- Emit migration warning when aliasing is used.

**Validation gate**
- Run full build/package/run/all matrix listed in this plan.
- If a command fails due to missing host tool (e.g., QEMU binary), classify as environment/tooling issue and document.

**Docs**
- Update build docs to mark `delivery/targets/qemu/` as preferred and `tools/targets/qemu/` as legacy alias.

**Current status**
- YAML targets already reside in `delivery/targets/qemu/`.
- Compatibility aliasing is active in `tools/build.py` target resolution.

---

### Phase B (active) — Tools root transition (`tools/targets` + `tools/ci` compatibility)

**Scope**
- Begin shifting non-runtime tooling assets to `delivery/targets` and `delivery/tools`.

**Code tasks**
- Introduce a reusable path-alias helper in Python tooling (`tools/build/*`, validators, lints).
- Keep a two-way fallback (`delivery/*` preferred, legacy root fallback).
- Add a CI check for new references to legacy roots in touched files.

**Execution order (medium chunks)**
1. Extract alias logic into a shared helper module and wire `target_resolver.py` to use it.
2. Apply the same helper to other target-consuming scripts (`tools/targets/loader.py`, validation scripts).
3. Add CI guard for newly introduced `tools/targets/qemu/` references.
4. Flip guard from warning to error after two migration phases.

**Validation**
- Same matrix + lint stage.
- Confirm warning visibility in CI logs.

---

### Phase C — Interface migration in thin slices

**Scope order**
1. `idl/` -> `interface/idl/` (completed),
2. `uapi/` -> `interface/uapi/` (completed),
3. remaining `sdk/` modules -> `interface/sdk/` (completed).

**Code tasks**
- One subtree move per PR.
- Keep include/tool fallbacks until end of Phase D.
- Update code generators and package manifests to consume new interface paths first.

**Validation**
- Matrix + contract validation + interface artifact generation checks.

---

### Phase D — Core runtime path migration in bounded units

**Recommended PR order**
1. `boot/` -> `core/boot/`
2. `kernel/` -> `core/kernel/`
3. `hal/` + `arch/` -> `core/hal/`, `core/arch/`
4. `platform/`, `drivers/`, `services/`, `stacks/`, `personalities/`, `lib/` -> `core/*`

**Code tasks per PR**
- Add include path compatibility at CMake level.
- Keep adapter include headers where path churn is high.
- Require compile + runtime checks before merge.

**Detailed medium-chunk execution (repo-aware)**
- **D1 (done in parts): boot canonicalization in `core/boot`**
  - D1a (already done): `boot/src` and `boot/common` moved to `core/boot`.
  - D1b (this slice): `boot/include`, `boot/discovery`, and `boot/protocols` moved to `core/boot/{include,discovery,protocols}`.
  - Compatibility retained with legacy wrappers in `boot/include/*`, `boot/discovery/*`, and `boot/protocols/*`.
  - Build wiring now resolves boot include/protocol roots through candidate lists (`core/boot/*` first, `boot/*` fallback with warning).
- **D2 (active): kernel tree move**
  - D2a (completed): moved `kernel/src/{init,core,boot}` into `core/kernel/src/*`.
  - `kernel/src/{init,core,boot}` remain as compatibility symlinks so legacy references still resolve.
  - `kernel/CMakeLists.txt` now resolves a kernel source-root candidate list (`core/kernel/src` preferred, `kernel/src` fallback with migration warning).
  - D2b (completed): moved `kernel/include` to `core/kernel/include` and retained `kernel/include` as compatibility symlink.
  - `kernel/CMakeLists.txt` now resolves kernel include roots from candidate list (`core/kernel/include` preferred, `kernel/include` fallback with migration warning).
  - D2c (completed): moved the remaining canonical kernel sources from `kernel/src/*` into `core/kernel/src/*`.
  - Legacy compatibility is preserved by keeping `kernel/src/*` entries as symlinks that forward to `core/kernel/src/*`, so existing references and scripts continue to resolve during transition.
  - D2e (completed): moved `kernel/staging/*` into canonical `core/kernel/staging/*` and inverted compatibility so legacy `kernel/staging` now forwards to the canonical tree via symlink.
  - Kernel CMake wiring now resolves staging sources through a canonical-first candidate list (`core/kernel/staging` preferred, `kernel/staging` fallback with migration warning).
- **D3 (active): arch + hal staged move**
  - D3a (completed): moved `arch/` to `core/arch/`; retained `arch` compatibility symlink so existing source references remain valid during transition.
  - Top-level build wiring now resolves architecture layer via `add_subdirectory(core/arch)` while legacy path references continue to resolve through the symlink.
  - D3b (completed): moved `hal/` to `core/hal/`; retained `hal` compatibility symlink so existing source references remain valid during transition.
  - Top-level build wiring now resolves the HAL layer via `add_subdirectory(core/hal)` while legacy path references continue to resolve through the symlink.
- **D4 (active): platform/services/lib ecosystem**
  - D4a (completed): moved `lib/` to `core/lib/` and `stacks/` to `core/stacks/`; retained `lib` and `stacks` as compatibility symlinks.
  - Top-level build wiring now prefers canonical `add_subdirectory(core/lib)` and `add_subdirectory(core/stacks)` while legacy path references still resolve through symlinks.
  - D4b (completed): moved `platform`, then `drivers`, `services`, and `personalities` to canonical `core/*` roots with compatibility symlinks retained during transition.

---

### Phase E — Cleanup and enforcement

**Tasks**
- Convert migration warnings to CI errors.
- Remove fallback aliases after all docs/scripts are updated.
- Enforce that new contributions use only zone-based paths.

**Done condition**
- Legacy paths absent from source, docs, and scripts.
