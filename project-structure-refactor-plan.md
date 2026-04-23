# Project Structure Refactor Plan

## Objective

Reduce top-level repository clutter by grouping folders into clear responsibility zones while preserving architecture boundaries and enabling incremental migration.

## Why refactor now

The architecture guidance already emphasizes:

- One folder = one primary responsibility.
- Separation of mechanism (`kernel`, `drivers`, `hal`) from policy (`services`, `stacks`, `personalities`).
- Contract-first integration using `uapi/` and `idl/`.

Current layout is functional but has too many root-level first-class concepts, increasing onboarding and ownership complexity.

## Proposed target model (zone-based root)

```text
Bharat-OS/
  core/            # OS runtime internals
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

  interface/       # external contracts + consumer-facing integration
    uapi/
    idl/
    sdk/
    contracts/
    include/       # only if truly public umbrella headers

  delivery/        # build, packaging, targets, release ops
    tools/
    cmake/
    configs/
    targets/
    release/
    assets/

  quality/
    tests/

  docs/
  .github/
```

## Mapping: current root -> proposed zone

| Current root | Proposed location | Notes |
| --- | --- | --- |
| `arch/` | `core/arch/` | ISA implementation remains unchanged; path moves only. |
| `boot/` | `core/boot/` | Keeps early boot with mechanism layer. |
| `hal/` | `core/hal/` | Supports HAL boundary cleanup initiative. |
| `platform/` | `core/platform/` | Board/machine integration stays close to core. |
| `kernel/` | `core/kernel/` | Keeps mechanism-centric internals grouped. |
| `drivers/` | `core/drivers/` | Driver taxonomy cleanup can proceed in place. |
| `services/` | `core/services/` | Continue consolidation into domain buckets. |
| `stacks/` | `core/stacks/` | Cross-layer subsystem composition remains centralized. |
| `personalities/` | `core/personalities/` | Compatibility/domain personality ownership retained. |
| `lib/` | `core/lib/` | Runtime/internal libraries stay near core execution path. |
| `uapi/` | `interface/uapi/` | Public contract surface. |
| `idl/` | `interface/idl/` | Interface definitions + generation source. |
| `sdk/` | `interface/sdk/` | Consumer and integration SDKs. |
| `contracts/` | `interface/contracts/` | ABI and protocol contract package area. |
| `include/` | `interface/include/` | Keep only externally consumable headers. |
| `tools/` | `delivery/tools/` | Build/dev/CI tooling bucket. |
| `cmake/` | `delivery/cmake/` | Build system modules and toolchain definitions. |
| `configs/` | `delivery/configs/` | Build/runtime configuration profiles. |
| `targets/` | `delivery/targets/` | Target matrix, platform presets. |
| `release/` | `delivery/release/` | Release manifests and packaging metadata. |
| `assets/` | `delivery/assets/` | Packaging/branding/runtime assets. |
| `tests/` | `quality/tests/` | Test ownership and quality domain clarity. |
| `docs/` | `docs/` | Stays at root for discoverability. |
| `.github/` | `.github/` | Stays at root for GitHub conventions. |

## Migration principles

1. **Do not break builds abruptly**: use temporary compatibility aliases in build scripts.
2. **Move low-coupling folders first** (`release`, `targets`, `contracts`, parts of `sdk`).
3. **Use contract gates**: no new cross-layer API without `uapi/idl` artifacts.
4. **Track ownership**: update CODEOWNERS and subsystem owners by zone.
5. **Protect kernel boundary**: extract policy logic from kernel paths where possible.

## Phased implementation plan

### Phase 0 - Prep (1-2 sprints)

- Publish approved target layout and ownership matrix.
- Add CI checks for forbidden new root-level directories.
- Add CI warning for new interfaces missing `uapi/idl` backing.

### Phase 1 - Non-disruptive scaffolding (1 sprint)

- Create `core/`, `interface/`, `delivery/`, `quality/` directories.
- Add build-system path aliases and deprecation notices.
- Keep old paths functional via indirection.

### Phase 2 - Low-risk moves (1-2 sprints)

- Move: `release`, `targets`, `contracts`, selected `sdk` subtrees.
- Update scripts, CI configs, docs references.
- Validate with full build/test matrix.

### Phase 3 - Core path migration (2-4 sprints)

- Move mechanism and policy trees under `core/`.
- Consolidate `services/*mgr` into domain buckets during migration.
- Begin HAL path normalization where architecture-specific internals are leaking abstraction.

### Phase 4 - Interface hardening (1-2 sprints)

- Move `uapi`, `idl`, remaining `sdk`, `include` under `interface/`.
- Enforce contract discipline for new APIs.
- Version and publish interface artifacts consistently.

### Phase 5 - Cleanup and lock (1 sprint)

- Remove temporary aliases.
- Fail CI on legacy paths.
- Update contributor onboarding docs and architecture snapshots.

## Risk register and mitigations

| Risk | Impact | Mitigation |
| --- | --- | --- |
| Broken imports/includes | High | Compatibility include paths + staged deprecation window. |
| Build script regressions | High | Path translation layer + CI matrix before each phase merge. |
| Tooling drift | Medium | Pin and update all scripts in one migration branch per phase. |
| Ownership ambiguity | Medium | CODEOWNERS by zone + explicit subsystem maintainers. |
| Merge conflicts during move | Medium | Short-lived migration branches + freeze windows for touched trees. |

## Definition of done

- Root-level directories reduced to zone-oriented buckets and required platform conventions.
- No active references to legacy paths in build scripts, CI, or docs.
- `services` consolidation and `uapi/idl` contract gate active in CI.
- Architecture folder structure document updated with post-migration alignment snapshot.

## Immediate next actions

1. Approve this plan and target mapping table.
2. Create tracking epic with one ticket per phase.
3. Start Phase 0 CI guardrails before any large move.
