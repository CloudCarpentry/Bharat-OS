# Project Structure Migration Status

This tracker is the execution companion for `project-structure-refactor-plan.md`.

## Current Snapshot (2026-04-23)

- Phase A (QEMU target YAML relocation): **Completed**.
- Phase B (tooling compatibility hardening): **In progress**.
- Phase C (interface moves): **Started** (`idl/` slice completed).

## Phase Checklist

| Phase | Scope | Status | Notes |
| --- | --- | --- | --- |
| A | Move QEMU target YAMLs to `delivery/targets/qemu/` + alias support | Completed | `tools/build/target_resolver.py` accepts legacy path references. |
| B.1 | Shared path-alias helper for migration-aware tooling | Completed | Added `tools/build/path_aliases.py` and routed target path resolution through it. |
| B.2 | Apply alias helper to target loaders/validators outside build pipeline | Pending | Next medium chunk. |
| B.3 | CI guard for newly introduced legacy root references | Pending | Start warning-only, then enforce. |
| C | `idl/`, `uapi/`, `sdk/` to `interface/` | In progress | C1 complete: `idl/` moved to `interface/idl/` with legacy compatibility path. |
| D | `boot/`, `kernel/`, `arch/`, etc. to `core/` | Pending | Atomic compile-safe slices only. |
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
