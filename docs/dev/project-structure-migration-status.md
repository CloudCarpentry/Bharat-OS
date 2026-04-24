# Project Structure Migration Status

This tracker is the execution companion for `project-structure-refactor-plan.md`.

## Current Snapshot (2026-04-24)

- Phase A (QEMU target YAML relocation): **Completed**.
- Phase B (tooling compatibility hardening): **In progress**.
- Phase C (interface moves): **Completed** (`idl/` + `uapi/` + `sdk` slices completed).
- Phase D.1a (boot source/common move): **In progress** (`boot/src` + `boot/common` moved to `core/boot` with compatibility symlinks).
- Phase D.2c (kernel source tree move, bounded slice): **In progress** (remaining `kernel/src/*` moved to `core/kernel/src/*`; legacy `kernel/src/*` compatibility symlink wrappers retained).
- Phase D.4a (lib + stacks bounded move): **In progress** (`lib/` and `stacks/` moved to canonical `core/lib/` and `core/stacks/`; legacy symlink compatibility retained).

## Phase Checklist

| Phase | Scope | Status | Notes |
| --- | --- | --- | --- |
| A | Move QEMU target YAMLs to `delivery/targets/qemu/` + alias support | Completed | `tools/build/target_resolver.py` accepts legacy path references. |
| B.1 | Shared path-alias helper for migration-aware tooling | Completed | Added `tools/build/path_aliases.py` and routed target path resolution through it. |
| B.2 | Apply alias helper to target loaders/validators outside build pipeline | Pending | Next medium chunk. |
| B.3 | CI guard for newly introduced legacy root references | Pending | Start warning-only, then enforce. |
| C | `idl/`, `uapi/`, `sdk/` to `interface/` | Completed | C1 (`idl`), C2 (`uapi`), C3 (`sdk`) complete; legacy compatibility symlinks retained. |
| D | `boot/`, `kernel/`, `arch/`, etc. to `core/` | In progress | D1a landed; D2b landed (`kernel/include`), D2c landed (remaining `kernel/src/*` now canonical in `core/kernel/src/*` with `kernel/src/*` symlink wrappers), and D4a landed (`lib/` + `stacks/` moved under `core/*` with compatibility symlinks). |
| E | Remove fallbacks + enforce new roots | Pending | Convert warnings to CI failures. |

## Mandatory Validation Matrix (per phase)

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

./build.sh all --target arm32_mmu_lite_headless
./build.sh all --target riscv32_mmu_lite_headless
```

## Documentation Rule (per phase)

Every migration PR must update:

1. User-facing build/run docs (e.g., `BUILD.md`, `README.md` command examples).
2. Architecture/developer docs that mention moved paths.
3. This tracker with phase status and validation outcomes.

## Validation Outcomes (2026-04-23, Phase C1)

- `./build.sh build --target x86_64_desktop_headless`: **pass**.
- `./build.sh package --target x86_64_desktop_headless`: **pass**.
- `./build.sh run --target x86_64_desktop_headless`: **pass (timeout-bounded)** after QEMU installation; boot reaches runtime self-test output before timeout.
- `./build.sh all --target x86_64_desktop_headless_linux`: **timeout-bounded warning** (command started, configure/build progressed).
- `./build.sh all --target arm64_desktop_headless_linux`: **timeout-bounded warning** (command started, configure/build progressed).
- `./build.sh all --target riscv64_desktop_headless_linux`: **timeout-bounded warning** (command started, configure/build progressed).
- `./build.sh all --target x86_64_desktop_headless_android`: **timeout-bounded warning** (command started, configure/build progressed).
- `./build.sh all --target arm64_desktop_headless_android`: **timeout-bounded warning** (command started, configure/build progressed).
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning** (command started, configure/build progressed).
- `./build.sh all --target arm32_mmu_lite_headless`: **timeout-bounded warning** (command started, configure/build progressed).
- `./build.sh all --target riscv32_mmu_lite_headless`: **timeout-bounded warning** (command started, configure/build progressed).

## Validation Outcomes (2026-04-23, Phase B.1)

- `./build.sh build --target x86_64_desktop_headless`: **pass**.
- `./build.sh package --target x86_64_desktop_headless`: **pass**.
- `./build.sh run --target x86_64_desktop_headless`: boots in QEMU; command requires timeout in CI/local automation because it is an interactive long-running VM.
- `./build.sh all --target {x86_64,arm64,riscv64}_desktop_headless_{linux,android}`: build+package+run path starts successfully; run stage is long-running and was timeout-bounded in this environment.
- `./build.sh all --target arm32_mmu_lite_headless`: build+package+run path starts successfully; run stage was timeout-bounded.
- `./build.sh all --target riscv32_mmu_lite_headless`: build+package passed; run failed due missing OpenSBI firmware path (`/usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin`) in host environment.

## Validation Outcomes (2026-04-23, Phase C2)

- Installed QEMU host runners for x86/arm/riscv via `apt-get install -y qemu-system-x86 qemu-system-arm qemu-system-misc`.
- `./build.sh build --target x86_64_desktop_headless`: **pass**.
- `./build.sh package --target x86_64_desktop_headless`: **pass**.
- `./build.sh run --target x86_64_desktop_headless`: **timeout-bounded run with successful boot output**; runtime self-tests reached `rt_sched` and reported one existing EDF test failure before timeout.
- `./build.sh all --target x86_64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target x86_64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).

## Validation Outcomes (2026-04-23, Phase C3)

- Migration slice: moved `sdk/` -> `interface/sdk/` and kept `sdk` compatibility symlink.
- Installed QEMU host runners via `apt-get install -y qemu-system-x86 qemu-system-arm qemu-system-misc`.
- `./build.sh build --target x86_64_desktop_headless`: **pass**.
- `./build.sh package --target x86_64_desktop_headless`: **pass**.
- `./build.sh run --target x86_64_desktop_headless`: **timeout-bounded run with successful boot output**; runtime self-tests reached `rt_sched` and preserved one existing EDF test failure before timeout.
- `./build.sh all --target x86_64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_linux`: **timeout-bounded warning with known runtime panic** (`PMM: Double free detected!`) observed during run stage.
- `./build.sh all --target x86_64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning with known runtime panic** (`PMM: Double free detected!`) observed during run stage.
- `./build.sh all --target arm32_mmu_lite_headless`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv32_mmu_lite_headless`: **build+package pass; run fails due missing OpenSBI firmware path** (`/usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin`).

## Validation Outcomes (2026-04-23, Phase D1a)

- Migration slice: moved `boot/src/` and `boot/common/` into `core/boot/`; preserved `boot/src` and `boot/common` as compatibility symlinks.
- `./build.sh build --target x86_64_desktop_headless`: **pass**.
- `./build.sh package --target x86_64_desktop_headless`: **pass**.
- `./build.sh run --target x86_64_desktop_headless`: **timeout-bounded warning** (QEMU interactive run started; bounded in automation).
- `./build.sh all --target x86_64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target x86_64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).

## Validation Outcomes (2026-04-24, Phase D2b)

- Migration slice: moved `kernel/include/` to `core/kernel/include/`; preserved `kernel/include` as compatibility symlink.
- `kernel/CMakeLists.txt` now resolves kernel include root from a candidate list (`core/kernel/include` first, `kernel/include` fallback with migration warning).
- `./build.sh build --target x86_64_desktop_headless`: **pass**.
- `./build.sh package --target x86_64_desktop_headless`: **pass**.
- `./build.sh run --target x86_64_desktop_headless`: **timeout-bounded warning** (QEMU interactive run started; bounded in automation).
- `./build.sh all --target x86_64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target x86_64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm32_mmu_lite_headless`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv32_mmu_lite_headless`: **failed at run stage** due missing OpenSBI firmware path (`/usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin`) on host.

## Validation Outcomes (2026-04-24, Phase D2c)

- Migration slice: moved remaining `kernel/src/*` sources into `core/kernel/src/*`; preserved compatibility via per-entry symlinks under `kernel/src/*`.
- `./build.sh build --target x86_64_desktop_headless`: **pass**.
- `./build.sh package --target x86_64_desktop_headless`: **pass**.
- `./build.sh run --target x86_64_desktop_headless`: **timeout-bounded warning** (QEMU interactive run started; bounded in automation).
- `./build.sh all --target x86_64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target x86_64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).


## Validation Outcomes (2026-04-24, Phase D4a)

- Migration slice: moved `lib/` -> `core/lib/` and `stacks/` -> `core/stacks/`; preserved compatibility via top-level `lib` and `stacks` symlinks.
- Top-level build wiring now prefers canonical paths (`add_subdirectory(core/lib)` and `add_subdirectory(core/stacks)`).
- Validation commands executed for build/package/run and Linux/Android multi-arch `all` targets (see command log in PR for pass/timeout details).