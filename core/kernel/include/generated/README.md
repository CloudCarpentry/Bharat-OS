# Why `core/kernel/include/generated/bharat_config.h` exists

`bharat_config.h` is **not handwritten configuration**. It is produced from
`core/kernel/include/bharat_config.h.in` via CMake `configure_file(...)` so that
kernel code can read one canonical set of compile-time flags (`BHARAT_PROFILE_*`,
`BHARAT_ENABLE_*`, `BHARAT_ARCH_*`, etc.).

## Why keep a generated header in-tree?

This repo currently supports two workflows:

1. **Top-level build** (`/CMakeLists.txt`) that creates generated artifacts in the
   build directory.
2. **Kernel-only configuration/build** (`core/kernel/CMakeLists.txt`) that still
   expects `bharat_config.h` to be available for include resolution in kernel
   subtargets.

Keeping this generated header under `core/kernel/include/generated/` acts as a
stable fallback/bootstrap artifact for tooling and partial builds.

## Pain points with committed generated files

- Merge conflicts and noisy diffs.
- Risk of stale values being reviewed as "source".
- Confusion about what should be edited (`*.h.in`) vs generated (`*.h`).

## Better options (brainstorm)

### Option A (recommended): build-dir generated only + compatibility shim

- Generate `bharat_config.h` into `${CMAKE_BINARY_DIR}/generated/include` only.
- Add include paths so all targets prefer build-dir generated headers.
- Keep a tiny in-tree shim header (or fail-fast stub) that points contributors to
  regenerate if they include it directly.

Pros: clean git history, single source of truth.
Cons: requires build system cleanup and CI updates.

### Option B: keep committed fallback snapshot with strict guardrails

- Keep the committed generated header.
- Add clear banner comments (`AUTO-GENERATED; DO NOT EDIT`).
- Add CI check to verify template + CMake values regenerate exact same content.

Pros: minimal migration effort.
Cons: still noisy, still easy to misunderstand.

### Option C: split stable vs dynamic config

- Commit a small stable `bharat_defaults.h` (rarely changing).
- Generate only dynamic profile/board-specific macros into build-dir header.
- Include both from kernel config entrypoint.

Pros: fewer generated deltas.
Cons: more moving parts and include order rules.

## Practical migration plan

1. Standardize include use to `#include "bharat_config.h"` from one exported
   generated include directory.
2. Move generation output to build dir only.
3. Add CI check for missing generation step with a clear error message.
4. Remove committed generated snapshot after one release cycle.

## Contributor rule

- Edit: `core/kernel/include/bharat_config.h.in`
- Do not edit manually: `core/kernel/include/generated/bharat_config.h`
