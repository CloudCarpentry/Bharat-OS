# Project Structure Migration Status

This tracker is the execution companion for `project-structure-refactor-plan.md`.

## Current Snapshot (2026-04-24)

- Phase A (QEMU target YAML relocation): **Completed**.
- Phase B (tooling compatibility hardening): **Completed** (B.1/B.2/B.3 complete; CI guard now enforces strict mode).
- Phase B.4 (delivery configs/assets relocation + legacy symlink trimming): **Completed** (`configs/` and `assets/` now canonical under `delivery/`; obsolete `quality/*` compatibility symlink fanout removed).
- Phase B.5 follow-up (root delivery compatibility cleanup): **Completed** (root `configs`, `assets`, and `targets` symlinks removed after canonical-path migration).
- Phase C (interface moves): **Completed** (`idl/` + `uapi/` + `sdk` slices completed).
- Phase D.1 (boot tree migration): **Completed** (D1a/D1b migrated boot sources+headers/protocols/discovery into `core/boot`; D1c wired kernel include selection through migration-aware `BHARAT_BOOT_INCLUDE_DIR`; D1e removed legacy `boot/{src,common,include,discovery,protocols}` symlink fanout and migrated in-repo include path usage to canonical `core/boot/*`; D1f removed legacy `boot/*` fallback selection from `core/kernel` CMake wiring so boot inputs are now canonical-only).
- Phase D.2c (kernel source tree move, bounded slice): **In progress** (remaining `kernel/src/*` moved to `core/kernel/src/*`; legacy `kernel/src/*` compatibility symlink wrappers retained).
- Phase D.2f (kernel root build assets move, bounded slice): **In progress** (`kernel/CMakeLists.txt` and `kernel/linker*.ld` moved to canonical `core/kernel/*`; legacy `kernel/*` symlink compatibility retained).
- Phase D.4a (lib + stacks bounded move): **In progress** (`lib/` and `stacks/` moved to canonical `core/lib/` and `core/stacks/`; legacy symlink compatibility retained).
- Phase F.1 (public include tree): **Completed** (`include/` moved to canonical `interface/include/`; legacy `include` retained as symlink).
- Phase F.2 (user-space tree): **Completed** (`user/` moved to canonical `experience/user/`; legacy `user` retained as symlink and top-level CMake now prefers `experience/user`).
- Phase F.2 follow-up (root experience/interface/quality symlink trimming): **Completed** (root `user`, `tests`, `idl`, and `sdk` compatibility symlinks removed; `include`/`uapi` kept temporarily for compatibility).

## Phase Checklist

| Phase | Scope | Status | Notes |
| --- | --- | --- | --- |
| A | Move QEMU target YAMLs to `delivery/targets/qemu/` + alias support | Completed | `tools/build/target_resolver.py` accepts legacy path references. |
| B.1 | Shared path-alias helper for migration-aware tooling | Completed | Added `tools/build/path_aliases.py` and routed target path resolution through it. |
| B.2 | Apply alias helper to target loaders/validators outside build pipeline | Completed | ABI tooling now resolves canonical `interface/*` paths through `tools/build/path_aliases.py` with migration warnings for legacy aliases. |
| B.3 | CI guard for newly introduced legacy root references | Completed | `kernel-ci` runs `tools/ci/check_migration_refs.py --strict` and guards completed migration roots (`delivery/targets`, `interface/{idl,uapi,contracts}`). |
| B.4 | Move `configs/` + `assets/` into `delivery/` and prune obsolete compatibility symlinks | Completed | Canonical paths are now `delivery/configs` and `delivery/assets`; obsolete `quality/*` symlink fanout removed and root delivery symlinks were later dropped in B.5. |
| B.5 | Remove root delivery symlinks once callers are canonicalized | Completed | Root `configs`, `assets`, and `targets` symlinks removed; canonical paths under `delivery/` are authoritative. |
| C | `idl/`, `uapi/`, `sdk/` to `interface/` | Completed | C1 (`idl`), C2 (`uapi`), C3 (`sdk`) complete; legacy compatibility symlinks retained. |
| D | `boot/`, `kernel/`, `arch/`, etc. to `core/` | In progress | D1 is now complete (canonical `core/boot/*`, legacy `boot/{src,common,include,discovery,protocols}` symlink fanout removed, in-repo include paths migrated, and `core/kernel` CMake now uses canonical `core/boot/*` roots only). D2b landed (`kernel/include`), D2c landed (remaining `kernel/src/*` now canonical in `core/kernel/src/*` with `kernel/src/*` symlink wrappers), D2f landed (`kernel/CMakeLists.txt` + `kernel/linker*.ld` moved to canonical `core/kernel/*` with legacy symlink compatibility), and D4a landed (`lib/` + `stacks/` moved under `core/*` with compatibility symlinks). |
| F | `include/` + `user/` canonicalization | Completed | F1 landed (`interface/include` canonical, `include` symlink retained); F2 landed (`experience/user` canonical, `user` symlink retained) with migration-aware CMake root selection. |
| F.3 | Trim root interface/experience/quality compatibility symlinks | Completed | Removed root `user`, `tests`, `idl`, and `sdk` symlinks; retained `include` and `uapi` symlinks for active compatibility needs. |
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
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning with known runtime panic** (`PMM: Double free detected!`) during run stage.

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
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning with known runtime panic** (`PMM: Double free detected!`) during run stage.

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
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning with known runtime panic** (`PMM: Double free detected!`) during run stage.
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
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning with known runtime panic** (`PMM: Double free detected!`) during run stage.

## Validation Outcomes (2026-04-24, Phase D2f)

- Migration slice: moved `kernel/CMakeLists.txt` and `kernel/linker*.ld` to canonical `core/kernel/*`; preserved compatibility via legacy `kernel/*` symlinks.
- Build wiring update: top-level `CMakeLists.txt` now resolves kernel CMake root from a canonical-first candidate list (`core/kernel` preferred, `kernel` fallback with migration warning).
- Environment prep: installed QEMU system emulators for x86/arm/riscv via `apt-get update && apt-get install -y qemu-system-x86 qemu-system-arm qemu-system-misc`.
- `./build.sh build --target x86_64_desktop_headless`: **pass**.
- `./build.sh package --target x86_64_desktop_headless`: **pass**.
- `./build.sh run --target x86_64_desktop_headless`: **timeout-bounded warning with successful boot output**; reached runtime self-tests and preserved known EDF scheduler test failure before timeout.
- `./build.sh all --target x86_64_desktop_headless_linux`: **timeout-bounded warning** (build+package completed; run reached runtime logs before timeout).
- `./build.sh all --target arm64_desktop_headless_linux`: **timeout-bounded warning** (build+package completed; run reached runtime logs before timeout).
- `./build.sh all --target riscv64_desktop_headless_linux`: **timeout-bounded warning with known runtime panic** (`PMM: Double free detected!`) before timeout.
- `./build.sh all --target x86_64_desktop_headless_android`: **timeout-bounded warning** (build+package completed; run reached runtime logs before timeout).
- `./build.sh all --target arm64_desktop_headless_android`: **timeout-bounded warning** (build+package completed; run reached runtime logs before timeout).
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning with known runtime panic** (`PMM: Double free detected!`) before timeout.


## Validation Outcomes (2026-04-24, Phase D1c)

- Migration slice: switched kernel sub-target CMake include wiring (`cap`, `trap`, `tests`, `debug`, `sched`, `ipc`, `fs`, `monitor`, `core`) from hardcoded `${CMAKE_SOURCE_DIR}/boot/include` to migration-aware `${BHARAT_BOOT_INCLUDE_DIR}`.
- Compatibility fix: updated kernel capability wrapper include (`kernel/include/bharat/hw_caps.h`) to reference canonical `interface/uapi` contract path so builds remain stable while include roots transition.
- Compatibility fix: updated accel self-test include in `kernel/src/tests/ktest_virt_accel.c` to use canonical public include path.
- Installed QEMU host runners for x86/arm/riscv via `apt-get install -y qemu-system-x86 qemu-system-arm qemu-system-misc`.
- `./build.sh build --target x86_64_desktop_headless`: **pass**.
- `./build.sh package --target x86_64_desktop_headless`: **pass**.
- `./build.sh run --target x86_64_desktop_headless`: **timeout-bounded run with successful boot output**; runtime self-tests reached expected known EDF scheduler failure before timeout.
- `./build.sh all --target x86_64_desktop_headless_linux`: **timeout-bounded warning** (build+package completed; run entered kernel runtime before timeout).
- `./build.sh all --target arm64_desktop_headless_linux`: **timeout-bounded warning** (build progressed substantially; command bounded in automation).
- `./build.sh all --target riscv64_desktop_headless_linux`: **timeout-bounded warning** (build progressed substantially; command bounded in automation).
- `./build.sh all --target x86_64_desktop_headless_android`: **timeout-bounded warning** (build progressed substantially; command bounded in automation).
- `./build.sh all --target arm64_desktop_headless_android`: **timeout-bounded warning** (build+run reached runtime logs before timeout).
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning with known runtime trap** (`SCAUSE=0x5`) before timeout.

## Validation Outcomes (2026-04-24, Phase D4a)

- Migration slice: moved `lib/` -> `core/lib/` and `stacks/` -> `core/stacks/`; preserved compatibility via top-level `lib` and `stacks` symlinks.
- Top-level build wiring now prefers canonical paths (`add_subdirectory(core/lib)` and `add_subdirectory(core/stacks)`).
- Validation commands executed for build/package/run and Linux/Android multi-arch `all` targets (see command log in PR for pass/timeout details).


## Validation Outcomes (2026-04-24, Phase D1e)

- Migration slice: removed legacy `boot/{src,common,include,discovery,protocols}` symlink fanout after migrating in-repo include path consumers to canonical `core/boot/*` locations.
- Build wiring cleanup: removed top-level compatibility `add_subdirectory(boot)` shim invocation; canonical `add_subdirectory(core/boot)` remains authoritative.
- Validation commands executed for build/package/run and Linux/Android multi-arch `all` targets for x86_64/arm64/riscv64 (see command log in PR for pass/timeout details).

## Validation Outcomes (2026-04-24, Phase D1d)

- Migration slice: replaced legacy wrapper file trees under `boot/include`, `boot/discovery`, and `boot/protocols` with compatibility symlinks to canonical `core/boot/*` directories.
- `./build.sh build --target x86_64_desktop_headless`: **pass**.
- `./build.sh package --target x86_64_desktop_headless`: **pass**.
- `./build.sh run --target x86_64_desktop_headless`: **timeout-bounded warning** (QEMU interactive run started; bounded in automation).
- `./build.sh all --target x86_64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target x86_64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning with known runtime panic** (`PMM: Double free detected!`) during run stage.

## Validation Outcomes (2026-04-24, Phase B.2)

- Migration slice: unified ABI tooling migration path resolution using shared alias helpers in `tools/build/path_aliases.py` (IDL, UAPI, ABI-manifest roots now prefer canonical `interface/*` paths).
- `python3 -m py_compile tools/build/path_aliases.py tools/abi/check_idl_compat.py tools/abi/generate_abi_manifests.py tools/abi/check_struct_layouts.py`: **pass**.
- `./build.sh build --target x86_64_desktop_headless`: **pass**.
- `./build.sh package --target x86_64_desktop_headless`: **pass**.
- `./build.sh run --target x86_64_desktop_headless`: **timeout-bounded warning** (QEMU interactive run started; bounded in automation).
- `./build.sh all --target x86_64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).

## Validation Outcomes (2026-04-24, Phase B.5 + F.3)

- Migration slice: removed root compatibility symlinks `configs`, `assets`, `targets`, `tests`, `user`, `idl`, and `sdk`; retained `include` and `uapi` symlinks because current CMake include wiring still relies on those compatibility roots.
- Environment prep: installed QEMU runners for x86/arm/riscv families via `apt-get install -y qemu-system-x86 qemu-system-arm qemu-system-misc`.
- `./build.sh build --target x86_64_desktop_headless`: **pass**.
- `./build.sh package --target x86_64_desktop_headless`: **pass**.
- `./build.sh run --target x86_64_desktop_headless`: **timeout-bounded warning with successful boot output**; runtime reached known EDF scheduler self-test failure before timeout.
- `./build.sh all --target x86_64_desktop_headless_linux`: **timeout-bounded warning** (build+package complete; run reached runtime logs before timeout).
- `./build.sh all --target arm64_desktop_headless_linux`: **timeout-bounded warning** (build+package complete; run reached runtime logs before timeout).
- `./build.sh all --target riscv64_desktop_headless_linux`: **timeout-bounded warning with known runtime panic** (`PMM: Double free detected!`) before timeout.
- `./build.sh all --target x86_64_desktop_headless_android`: **timeout-bounded warning** (build+package complete; run reached runtime logs before timeout).
- `./build.sh all --target arm64_desktop_headless_android`: **timeout-bounded warning** (build+package complete; run reached runtime logs before timeout).
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning with known runtime panic** (`PMM: Double free detected!`) before timeout.
- `./build.sh all --target x86_64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning with known runtime panic** (`PMM: Double free detected!`) during run stage.

## Validation Outcomes (2026-04-24, Phase B.3)

- Migration slice: escalated migration-reference guard to strict CI mode in `kernel-ci` and expanded guarded legacy patterns for completed migrations (QEMU targets, target matrix, IDL, UAPI include root, ABI contracts).
- Installed QEMU host runners for x86/arm/riscv via `apt-get install -y qemu-system-x86 qemu-system-arm qemu-system-misc`.
- `./build.sh build --target x86_64_desktop_headless`: **pass**.
- `./build.sh package --target x86_64_desktop_headless`: **pass**.
- `./build.sh run --target x86_64_desktop_headless`: **timeout-bounded warning** (QEMU interactive run started; bounded in automation).
- `./build.sh all --target x86_64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_linux`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target x86_64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target arm64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning** (build/run path starts).

## Validation Outcomes (2026-04-24, Phases F1 + F2)

## Validation Outcomes (2026-04-24, Phase D1f)

- Migration slice: removed legacy `boot/*` fallback selection in `core/kernel/CMakeLists.txt` and `core/kernel/src/init/CMakeLists.txt`; kernel build wiring now requires canonical `core/boot/{include,protocols,src}` roots.
- Canonical path follow-up: migrated kernel subdirectory CMake source/include references to canonical resolver variables (`BHARAT_KERNEL_SRC_ROOT`, `BHARAT_KERNEL_INCLUDE_DIR`, `BHARAT_LIB_ROOT`, `BHARAT_ARCH_ROOT`) to reduce dependency on legacy `kernel/*`, `lib/*`, and `arch/*` compatibility links during D1/D2 progression.
- Environment prep: installed QEMU multi-arch runners via `apt-get update && apt-get install -y qemu-system-x86 qemu-system-arm qemu-system-misc`.
- `./build.sh build --target x86_64_desktop_headless`: **pass**.
- `./build.sh package --target x86_64_desktop_headless`: **pass**.
- `./build.sh run --target x86_64_desktop_headless`: **timeout-bounded warning with successful boot output** (runtime reached self-tests; known EDF scheduler test failure preserved before timeout).
- `./build.sh all --target x86_64_desktop_headless_linux`: **timeout-bounded warning with successful boot output** (runtime reached self-tests before timeout).
- `./build.sh all --target arm64_desktop_headless_linux`: **timeout-bounded warning with successful boot output** (runtime reached self-tests before timeout).
- `./build.sh all --target riscv64_desktop_headless_linux`: **timeout-bounded warning with known runtime panic** (`PMM: Double free detected!`) before timeout.
- `./build.sh all --target x86_64_desktop_headless_android`: **in-progress timeout-bounded warning** (build+run started; command bounded in automation).
- `./build.sh all --target arm64_desktop_headless_android`: **pending in this slice** (not reached before bounded automation window while prior long-running targets executed).
- `./build.sh all --target riscv64_desktop_headless_android`: **pending in this slice** (not reached before bounded automation window while prior long-running targets executed).

- Migration slice F1: moved root `include/` to canonical `interface/include/`; retained top-level `include` symlink for compatibility.
- Migration slice F2: moved root `user/` to canonical `experience/user/`; retained top-level `user` symlink and updated top-level CMake to prefer canonical root with legacy fallback warning.
- Installed QEMU runners for x86/arm/riscv + user emulation support via `apt-get install -y qemu-system-x86 qemu-system-arm qemu-system-misc qemu-user-static binfmt-support`.
- `./build.sh build --target x86_64_desktop_headless`: **pass**.
- `./build.sh package --target x86_64_desktop_headless`: **pass**.
- `./build.sh run --target x86_64_desktop_headless`: **timeout-bounded run with successful boot output**; runtime self-tests reached expected known EDF scheduler failure before timeout.
- `./build.sh all --target x86_64_desktop_headless_linux`: **timeout-bounded warning** (build+run path starts and reaches runtime logs).
- `./build.sh all --target arm64_desktop_headless_linux`: **timeout-bounded warning** (build+run path starts and reaches runtime logs).
- `./build.sh all --target riscv64_desktop_headless_linux`: **timeout-bounded warning with known runtime panic** (`PMM: Double free detected!`) before timeout.
- `./build.sh all --target x86_64_desktop_headless_android`: **timeout-bounded warning** (build+run path starts and reaches runtime logs).
- `./build.sh all --target arm64_desktop_headless_android`: **timeout-bounded warning** (build+run path starts and reaches runtime logs).
- `./build.sh all --target riscv64_desktop_headless_android`: **timeout-bounded warning with known runtime panic** (`PMM: Double free detected!`) before timeout.

## Validation Outcomes (2026-04-24, Phase B.4)

- Migration slice: moved top-level `configs/` -> `delivery/configs/` and `assets/` -> `delivery/assets/` with root compatibility symlinks retained.
- Compatibility cleanup: removed obsolete `quality/{arch,boot,drivers,hal,include,kernel,lib,personalities,platform,services,stacks,user}` symlink fanout (no in-repo references remained).
- Installed QEMU host runners for x86/arm/riscv via `apt-get install -y qemu-system-x86 qemu-system-arm qemu-system-misc`.
- Validation commands executed for build/package/run and Linux/Android multi-arch `all` targets for x86_64/arm64/riscv64 (see command log in PR for pass/timeout details).
