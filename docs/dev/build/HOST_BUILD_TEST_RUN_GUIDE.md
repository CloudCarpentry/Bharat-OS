# Build System Guide (Authoritative)

This is the current, code-aligned guide for Bharat-OS build tooling.

It documents:

- the Python-first build pipeline (`tools/build.py`),
- wrapper behavior (`build.sh`, `build.ps1`),
- legacy CLI compatibility (`--run`, `--build`, `--all`),
- YAML targets and legacy target names,
- build/package/run/flash/debug stages,
- CMake preset + test preset workflows,
- headless target usage and common flags.

---

## 1) Current Build Entry Points

Use one of these from repository root:

```bash
./build.sh <subcommand> --target <target_name>
./build.sh <subcommand> --target-yaml <path/to/target.yaml>
```

```powershell
.\build.ps1 <subcommand> --target <target_name>
.\build.ps1 <subcommand> --target-yaml <path/to/target.yaml>
```

Both wrappers are compatibility shims that call `tools/build.py`.

### Supported subcommands

- `configure`
- `build`
- `package`
- `run`
- `flash`
- `debug`
- `all` (build + package + run)

### Supported options

- `--target <name>` (legacy target registry via `build_config.json`)
- `--target-yaml <path>` (explicit YAML target)
- `--dry-run` (only for `flash`)

---

## 2) Legacy CLI Compatibility (Important)

Legacy positional invocation is still accepted by parser compatibility logic, for example:

```bash
./build.sh x86_64_desktop_headless --run
./build.sh x86_64_desktop_headless --all
./build.sh x86_64_desktop_headless --build
```

Behavior mapping:

- `--run` or `--all` maps internally to `all --target ...`
- `--build` maps internally to `build --target ...`
- A warning is printed and this mode is considered compatibility-only.

Also tolerated in legacy mode:

```bash
./build.sh x86_64_desktop_headless -- run
./build.sh x86_64_desktop_headless -- all
```

Use explicit subcommands for all new docs, scripts, and CI.

---

## 3) YAML Target Support

Bharat-OS supports explicit YAML targets under `tools/targets/qemu/`.

Current examples include:

- `x86_64_desktop_headless`
- `arm64_desktop_headless`
- `riscv64_desktop_headless`
- `arm32_mmu_lite_headless`
- `riscv32_mmu_lite_headless`
- `arm32_micro`, `arm32_small`
- `riscv32_micro`, `riscv32_small`

Example usage:

```bash
./build.sh all --target-yaml tools/targets/qemu/x86_64_desktop_headless.yaml
```

### Why YAML targets matter

YAML targets define:

- CMake preset (`build.cmake_preset`),
- CMake overrides (`build.cmake_defs`),
- kernel artifact contract,
- package transforms (for example multiboot ELF fixups),
- run backend + machine/cpu/memory/serial/display mode,
- optional flash/debug contracts.

This is now the recommended path for scalable target growth.

---

## 4) Build Stage Semantics

### `configure`
Runs CMake configure only for the resolved preset.

### `build`
Runs configure + compile and writes build manifest metadata.

### `package`
Generates output manifests (run/flash/debug) and packaging transforms.

### `run`
Packages as needed, then executes target runtime backend (QEMU path for current QEMU targets).

### `all`
Equivalent to build + package + run pipeline for the target.

### `flash`
Executes flash backend for targets that include flash config. `--dry-run` supported.

### `debug`
Validates debug-capable target path; full automation is not yet complete.

---

## 5) Headless, GUI, and Runtime Behavior

Most primary automation flows should use headless targets.

### Recommended headless smoke commands

```bash
./build.sh all --target x86_64_desktop_headless
./build.sh all --target arm64_desktop_headless
./build.sh all --target riscv64_desktop_headless
./build.sh all --target arm32_mmu_lite_headless
./build.sh all --target riscv32_mmu_lite_headless
```

PowerShell equivalent:

```powershell
.\build.ps1 all --target x86_64_desktop_headless
.\build.ps1 all --target arm64_desktop_headless
.\build.ps1 all --target riscv64_desktop_headless
.\build.ps1 all --target arm32_mmu_lite_headless
.\build.ps1 all --target riscv32_mmu_lite_headless
```

---

## 6) CMake Presets and Test Presets

Bharat-OS uses `CMakePresets.json` extensively.

### Inspect available presets

```bash
cmake --list-presets
cmake --list-presets=build
cmake --list-presets=test
```

### Common test workflow

```bash
cmake --preset host-test
cmake --build --preset host-test
ctest --preset host-test
```

### Sanitizer and analysis-friendly presets

```bash
cmake --preset host-test-asan
cmake --build --preset host-test-asan
ctest --preset host-test-asan

cmake --preset host-test-valgrind
cmake --build --preset host-test-valgrind
ctest --preset host-test-valgrind
```

### CI-focused preset

```bash
cmake --preset ci-smoke
cmake --build --preset ci-smoke
ctest --preset ci-smoke
```

Note: in this repo, many historical docs used `tests-host`/`build-tests`/`run-tests`; the canonical preset names are those defined in `CMakePresets.json`.

---

## 7) Architecture Quick Commands

See architecture quick guides in this same folder:

- `BUILD_X86_64.md`
- `BUILD_ARM64.md`
- `BUILD_RISCV64.md`

---

## 8) Troubleshooting

- If target resolution fails, verify target name or YAML path.
- If run fails, verify architecture-specific `qemu-system-*` binary is installed.
- If preset errors occur, verify `cmake --list-presets` and CMake >= 3.20.
- If flash fails, check that selected target includes flash backend and toolchain.
- If legacy syntax behaves unexpectedly, switch to explicit subcommand syntax immediately.
