# Bharat-OS Build, Package, Run, and Debug Guide

This document is the **single user guide** for using Bharat-OS build tooling on:

- Windows (PowerShell)
- WSL/Linux
- macOS

It covers:

- what to install,
- how `build.ps1` / `build.sh` work,
- how to build/package/run/debug,
- QEMU workflows,
- board flashing workflows,
- preset command recipes (especially headless).

---

## Nirmaan CLI

We provide the `nirmaan` Developer CLI as the primary, high-productivity interface for managing builds and environment configurations.

```bash
# Check the environment
./nirmaan doctor

# List available shortcut targets
./nirmaan targets

# Build the desktop-x86_64 target with specific modes
./nirmaan build desktop-x86_64 --mode development
./nirmaan build desktop-x86_64 --mode release

# Run the target
./nirmaan run desktop-x86_64

# For Windows users:
nirmaan.bat doctor
.\nirmaan.ps1 doctor

## Nirmaan command cookbook

### Daily commands

```powershell
# Windows PowerShell
.\nirmaan.ps1 build --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke
.\nirmaan.ps1 build --target-yaml delivery/targets/qemu/x86_64_desktop_gui.yaml
.\nirmaan.ps1 build --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml --smoke
.\nirmaan.ps1 build --target-yaml delivery/targets/qemu/arm64_desktop_gui.yaml
.\nirmaan.ps1 build --target-yaml delivery/targets/qemu/riscv64_desktop_headless.yaml --smoke
.\nirmaan.ps1 run --target-yaml delivery/targets/qemu/riscv64_desktop_gui.yaml --interactive
.\nirmaan.ps1 build --target-yaml delivery/targets/qemu/arm32_mmu_lite_headless.yaml --smoke
.\nirmaan.ps1 build --target-yaml delivery/targets/qemu/riscv32_mmu_lite_headless.yaml --smoke
# MPU-Only Headless (RTOS Profile)
.\nirmaan.ps1 build --target-yaml delivery/targets/qemu/arm32_rtos_mpu_headless.yaml --smoke
.\nirmaan.ps1 build --target-yaml delivery/targets/qemu/riscv32_rtos_mpu_headless.yaml --smoke
# 64-bit MMU-Lite RTOS Headless
.\nirmaan.ps1 build --target-yaml delivery/targets/qemu/arm64_rtos_mmu_lite_headless.yaml --smoke
.\nirmaan.ps1 build --target-yaml delivery/targets/qemu/riscv64_rtos_mmu_lite_headless.yaml --smoke
.\nirmaan.ps1 build --target-yaml delivery/targets/qemu/x86_64_rtos_mmu_lite_headless.yaml --smoke
```

### One-shot run commands

```bash
# Linux/macOS/WSL
./nirmaan run --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke
./nirmaan run --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml --smoke
```

> [!NOTE] The `nirmaan` CLI mirrors the functionality of the legacy `build` wrappers; it forwards to `tools/build.py` internally.

> **Tip:** `--interactive` is a flag for the `run` subcommand only. When using `build`, omit `--interactive`.

### Running the HMEM demo

After building the demo image, you can launch it interactively for a specific architecture:

```powershell
# x86_64
.\\nirmaan.ps1 run --target-yaml delivery/targets/qemu/x86_64_hmem_demo.yaml --interactive
# arm64
.\\nirmaan.ps1 run --target-yaml delivery/targets/qemu/arm64_hmem_demo.yaml --interactive
# riscv64
.\\nirmaan.ps1 run --target-yaml delivery/targets/qemu/riscv64_hmem_demo.yaml --interactive
```

Use `--headless` instead of `--interactive` for a non‑GUI execution.


### HMEM base demo

The HMEM demo showcases heterogeneous memory support in QEMU. It builds a minimal kernel+userspace image and runs a benchmark that allocates memory from both DRAM and simulated HBM.

```powershell
# Build the demo image for each architecture
.\nirmaan.ps1 build --target-yaml delivery/targets/qemu/x86_64_hmem_demo.yaml --smoke
.\nirmaan.ps1 build --target-yaml delivery/targets/qemu/arm64_hmem_demo.yaml --smoke
.\nirmaan.ps1 build --target-yaml delivery/targets/qemu/riscv64_hmem_demo.yaml --smoke

# Run the demo (interactive QEMU window) for each architecture
.\nirmaan.ps1 run --target-yaml delivery/targets/qemu/x86_64_hmem_demo.yaml --interactive
.\nirmaan.ps1 run --target-yaml delivery/targets/qemu/arm64_hmem_demo.yaml --interactive
.\nirmaan.ps1 run --target-yaml delivery/targets/qemu/riscv64_hmem_demo.yaml --interactive
```

> **Note:** The demo requires QEMU version ≥ 8.0 with `-machine memdev` support and the `hmem` device enabled. Ensure the `examples/hmem_demo/` directory is present in the repository (added by this change).

## 1) Legacy tool entrypoints and command model

Bharat-OS provides two user-facing wrappers:

- `./build.sh` (Linux/macOS/WSL)
- `.\build.ps1` (Windows PowerShell)

Both wrappers forward directly to `tools/build.py`. The Python CLI is authoritative and supports these subcommands:

- `configure`
- `build`
- `package`
- `run`
- `flash`
- `debug`
- `all`

Each subcommand requires exactly one target selector:

- `--target <name>` (legacy targets from `build_config.json`)
- `--target-yaml <path>` (explicit target YAML; preferred under `delivery/targets/`)

Examples:

```powershell
# Windows PowerShell
.\tools\build.ps1 build --target x86_64_desktop_headless
.\tools\build.ps1 all --target x86_64_desktop_headless
.\tools\build.ps1 run --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml
```

```bash
# Linux/macOS/WSL
./tools/build.sh build --target x86_64_desktop_headless
./tools/build.sh all --target x86_64_desktop_headless
./tools/build.sh run --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml
```

> Legacy positional syntax still works (for example `.\build.ps1 x86_64_desktop_headless --run`), but it is compatibility-only and emits a warning. Prefer explicit subcommands.

### Transitional path compatibility (active migration behavior)

- Preferred target YAML location: `delivery/targets/qemu/*.yaml`
- Preferred target matrix location: `delivery/targets/target_matrix.json`
- Legacy paths under `tools/targets/qemu` and `targets/` are still accepted through alias translation during migration phases and emit migration warnings where applicable.

---

## 2) Host prerequisites by platform

### Windows host (PowerShell)

Install:

- Python 3
- CMake (3.20+)
- Ninja
- LLVM/Clang + LLD (and `llvm-objcopy`)
- QEMU
- (optional for board flashing) OpenOCD
- (optional for debug) GDB (`gdb` or `gdb-multiarch`)

Typical installs (examples):

```powershell
# Winget examples
winget install Python.Python.3
winget install Kitware.CMake
winget install Ninja-build.Ninja
winget install LLVM.LLVM
winget install SoftwareFreedomConservancy.QEMU
```

Ensure these commands are in `PATH`: `python`, `cmake`, `ninja`, `clang`, `ld.lld`, and the needed `qemu-system-*` binary.

### WSL / Linux host

Install:

- `python3`
- `cmake`
- `ninja-build`
- `clang`, `lld`, `llvm` (for `llvm-objcopy`)
- `qemu-system-x86`, `qemu-system-arm`, `qemu-system-misc`
- OpenSBI firmware for the RISC-V QEMU runners
- (optional) `openocd` for board flashing
- (optional) `gdb-multiarch`

The RV32 runner expects QEMU's standard
`opensbi-riscv32-generic-fw_dynamic.bin` firmware. Some distributions package
only the RV64 image; install the upstream OpenSBI ILP32 generic firmware into
QEMU's firmware search directory before running the RV32 smoke targets.

```bash
sudo apt update
sudo apt install -y \
  python3 cmake ninja-build clang lld llvm \
  qemu-system-x86 qemu-system-arm qemu-system-misc \
  opensbi openocd gdb-multiarch
```

### macOS host

Install with Homebrew:

- `python`
- `cmake`
- `ninja`
- `llvm`
- `qemu`
- (optional) `openocd`
- (optional) `gdb`

```bash
brew install python cmake ninja llvm qemu openocd gdb
```

If Homebrew LLVM is not default, export PATH:

```bash
export PATH="$(brew --prefix llvm)/bin:$PATH"
```

---

## 3) Build pipeline stages (what each command does)

### `configure`

Runs CMake configure preset and writes build manifest metadata.

### `build`

Runs configure + compile for the selected preset.

### `package`

Generates packaging artifacts/manifests (run/flash/debug manifests + footprint report).

### `run`

Packages (if needed) and launches QEMU for the target.

### `all`

Build + package + run in one command.

### `flash`

Packages and runs flashing backend (currently OpenOCD backend path).

### `debug`

Generates/validates debug path, but current workflow reports that full debug automation is not yet implemented.

Output layout uses CMake preset name:

- `build/<preset>/...`
- `build/<preset>/manifests/run-manifest.json`
- `build/<preset>/manifests/flash-manifest.json`
- `build/<preset>/manifests/debug-manifest.json`

### Build instrumentation versus product configuration

`CMAKE_BUILD_TYPE` selects one of three instrumentation values and does not
select an OS target or profile:

| CMake build type | Instrumentation value | Symbols | Optimization | Assertions/invariant checks |
| ---------------- | --------------------- | ------- | ------------ | --------------------------- |
| `Debug`          | `DEBUG`               | on      | unoptimized  | on                          |
| `RelWithDebInfo` | `RELWITHDEBINFO`      | on      | optimized    | off                         |
| `Release`        | `RELEASE`             | off     | optimized    | off                         |

The canonical x86_64 QEMU desktop target is factored through the hidden
`x86_64-qemu-desktop-base` preset. Its Debug, RelWithDebInfo, and Release
presets inherit exactly the same x86_64 / QEMU / DESKTOP / MMU_FULL / GP /
NATIVE product selection. Each configure writes
`build/<preset>/generated/build-configuration.json`; CI compares variants with:

```bash
python3 tools/check_build_variant_equivalence.py \
  build/x86_64-qemu-desktop-debug/generated/build-configuration.json \
  build/x86_64-qemu-desktop-release/generated/build-configuration.json
```

The checker fails on any functional difference. Differences are permitted only
inside the explicit instrumentation object (`variant`, assertions, symbols,
optimization, tracing, test hooks, poisoning, and invariant checking).

Audit findings retained outside this P0 canonical-target change:

- The older `x86_64-debug` preset selects board `host` and host tests while
  `x86_64-dev` selects QEMU; they are distinct products and must not be treated
  as a Debug/Release pair.
- `tiny-mpu-debug` enables allocation classes while `tiny-mpu-release` disables
  them. A follow-up must factor that target through a shared product preset.
- No build-type condition in target YAML resolution changes architecture,
  memory model, scheduler profile, personality, runtime model, or services.
  YAML supplies a preset plus explicit CMake definitions independent of build
  type.
- `HARDENED_RELEASE` is not defined: hardening flags and their supported
  toolchain/backend matrix need a separate contract rather than scope expansion
  in this task.

---

## 4) QEMU usage (desktop/headless/emulator targets)

### Recommended quick workflow

```powershell
.\build.ps1 all --target x86_64_desktop_headless
```

```bash
./build.sh all --target x86_64_desktop_headless
```

### Supported QEMU runners by architecture

- `x86_64` -> `qemu-system-x86_64`
- `arm64` -> `qemu-system-aarch64`
- `riscv64` -> `qemu-system-riscv64`
- `arm32` -> `qemu-system-arm`
- `riscv32` -> `qemu-system-riscv32`

Runtime command includes serial-first bring-up (`-nographic -monitor none -serial stdio`) so headless logs stream to your terminal.

### Run Modes (Smoke vs Interactive)

- **Smoke mode (`--smoke`):** QEMU exits automatically once the boot success marker is detected or a timeout is reached. Useful for CI.
- **Interactive mode (`--interactive`):** QEMU stays open until manually closed. Default for GUI targets.

### Display Overrides

- **`--gui`:** Force graphical display enabled.
- **`--headless`:** Force graphical display disabled (`-nographic`).

---

## 5) Preset command cookbook

### Daily commands

```powershell
# PowerShell
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/x86_64_desktop_gui.yaml --interactive
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm64_desktop_gui.yaml --interactive
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv64_desktop_headless.yaml --smoke
.\tools\build.ps1 run --target-yaml delivery/targets/qemu/riscv64_desktop_gui.yaml --interactive
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm32_mmu_lite_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv32_mmu_lite_headless.yaml --smoke
# MPU-Only Headless (RTOS Profile)
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm32_rtos_mpu_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv32_rtos_mpu_headless.yaml --smoke
# 64-bit MMU-Lite RTOS Headless
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm64_rtos_mmu_lite_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv64_rtos_mmu_lite_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/x86_64_rtos_mmu_lite_headless.yaml --smoke
```

```bash
# WSL/Linux/macOS
./tools/build.sh all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/x86_64_desktop_gui.yaml --interactive
./tools/build.sh run --target-yaml delivery/targets/qemu/x86_64_showcase_gui.yaml --interactive
./tools/build.sh all --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/arm64_desktop_gui.yaml --interactive
./tools/build.sh all --target-yaml delivery/targets/qemu/riscv64_desktop_headless.yaml --smoke
./tools/build.sh run --target-yaml delivery/targets/qemu/riscv64_desktop_gui.yaml --interactive
./tools/build.sh all --target-yaml delivery/targets/qemu/arm32_mmu_lite_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/riscv32_mmu_lite_headless.yaml --smoke
# MPU-Only Headless (RTOS Profile)
./tools/build.sh all --target-yaml delivery/targets/qemu/arm32_rtos_mpu_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/riscv32_rtos_mpu_headless.yaml --smoke
# 64-bit MMU-Lite RTOS Headless
./tools/build.sh all --target-yaml delivery/targets/qemu/arm64_rtos_mmu_lite_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/riscv64_rtos_mmu_lite_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/x86_64_rtos_mmu_lite_headless.yaml --smoke
```

The showcase target also supports a CI-friendly end-to-end check without a
window server. It still creates the emulated display and requires the real
framebuffer, splash/dashboard render, synthetic app interaction, and stable
userspace boot markers before passing:

```bash
./tools/build.sh all --target-yaml delivery/targets/qemu/x86_64_showcase_gui.yaml --headless --smoke
```

For pixel-level evidence, use the GUI visual smoke harness. Unlike the serial
smoke command, this command builds and packages from the target YAML, starts a
windowless QEMU display with private serial and QMP channels, waits for the
display and first-present markers, and validates a QMP `screendump` against the
mode reported by the guest:

```bash
python3 tools/test/qemu_gui_smoke.py \
  --target delivery/targets/qemu/x86_64_showcase_gui.yaml
```

The test rejects malformed captures, dimension mismatches, fewer than eight
distinct RGB colors, or a frame where less than one percent of pixels differ
from the dominant color. These thresholds may be made stricter with
`--minimum-colors` and `--minimum-non-dominant-ratio`; lowering them is intended
only for focused diagnosis, not for CI. The default evidence directory is
`build/<preset>/artifacts/gui-smoke/` and contains `first-frame.ppm`,
`serial.log`, and `qemu.log`. These are generated evidence and must not be
committed.

## 5.1 Canonical headless smoke-test commands (all 5 architectures)

All commands verified with `[Run] PASS` on QEMU. Build + package + run in one shot.

| Architecture | Profile       | Memory Model | Target YAML                           |
| ------------ | ------------- | ------------ | ------------------------------------- |
| x86_64       | Desktop GP    | MMU-Full     | `x86_64_desktop_headless.yaml`        |
| arm64        | Desktop GP    | MMU-Full     | `arm64_desktop_headless.yaml`         |
| riscv64      | Desktop GP    | MMU-Full     | `riscv64_desktop_headless.yaml`       |
| arm32        | Edge MMU-Lite | MMU-Lite     | `arm32_mmu_lite_headless.yaml`        |
| riscv32      | Edge MMU-Lite | MMU-Lite     | `riscv32_mmu_lite_headless.yaml`      |
| arm32        | RTOS MPU      | MPU-Only     | `arm32_rtos_mpu_headless.yaml`        |
| riscv32      | RTOS MPU      | MPU-Only     | `riscv32_rtos_mpu_headless.yaml`      |
| arm64        | RTOS MMU-Lite | MMU-Lite     | `arm64_rtos_mmu_lite_headless.yaml`   |
| riscv64      | RTOS MMU-Lite | MMU-Lite     | `riscv64_rtos_mmu_lite_headless.yaml` |
| x86_64       | RTOS MMU-Lite | MMU-Lite     | `x86_64_rtos_mmu_lite_headless.yaml`  |

```powershell
# PowerShell (Windows)

# 64-bit desktop-class targets (MMU-Full)
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv64_desktop_headless.yaml --smoke

# 32-bit edge/embedded targets (MMU-Lite)
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm32_mmu_lite_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv32_mmu_lite_headless.yaml --smoke

# MPU-Only targets
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm32_rtos_mpu_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv32_rtos_mpu_headless.yaml --smoke

# 64-bit MMU-Lite targets
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm64_rtos_mmu_lite_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv64_rtos_mmu_lite_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/x86_64_rtos_mmu_lite_headless.yaml --smoke
```

## 5.2 Canonical headless run commands (WSL/Linux/macOS)

### Personality Targets (Linux & Android)

You can explicitly build the desktop GP profile with a specific personality:

```bash
./tools/build.sh all --target x86_64_desktop_headless_linux
./tools/build.sh all --target x86_64_desktop_headless_android
```

These test targets assert that the ABI boundaries and dispatch tables do not cause build breakage or kernel panics during early boot.

```bash
./tools/build.sh all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/arm32_desktop_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/riscv64_desktop_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/riscv32_desktop_headless.yaml --smoke
```

## 5.3 GUI presets (examples)

```powershell
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/x86_64_desktop_gui.yaml --interactive
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm64_desktop_gui.yaml --interactive
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm32_desktop_gui.yaml --interactive
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv64_desktop_gui.yaml --interactive
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv32_desktop_gui.yaml --interactive
```

```bash
./tools/build.sh all --target-yaml delivery/targets/qemu/x86_64_desktop_gui.yaml --interactive
./tools/build.sh all --target-yaml delivery/targets/qemu/arm64_desktop_gui.yaml --interactive
./tools/build.sh all --target-yaml delivery/targets/qemu/arm32_desktop_gui.yaml --interactive
./tools/build.sh all --target-yaml delivery/targets/qemu/riscv64_desktop_gui.yaml --interactive
./tools/build.sh all --target-yaml delivery/targets/qemu/riscv32_desktop_gui.yaml --interactive
```

## 5.4 Legacy positional example requested by users

```powershell
.\build.ps1 x86_64_desktop_headless --run
```

Equivalent modern command:

```powershell
.\build.ps1 all --target x86_64_desktop_headless
```

---

## 6) Package-only and run-only workflows

```powershell
.\build.ps1 build --target x86_64_desktop_headless
.\build.ps1 package --target x86_64_desktop_headless
.\build.ps1 run --target x86_64_desktop_headless
```

```bash
./tools/build.sh build --target x86_64_desktop_headless
./tools/build.sh package --target x86_64_desktop_headless
./tools/build.sh run --target x86_64_desktop_headless
```

YAML target path equivalent:

```bash
./build.sh all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml
```

---

## 7) Debug workflow

---

## 8) Board and hardware flashing workflow

---

## 9) Discovering available presets and targets

---

## 10) Matrix commands

You can build and test all QEMU targets at once:

```bash
# Build and smoke-test all headless targets
python3 tools/run_qemu_matrix.py --headless --smoke

# Build all GUI targets without launching
python3 tools/run_qemu_matrix.py --gui --build-only
```

### Documentation and Profile Consistency

To ensure that all build profiles are properly documented in the README and architecture docs:

```bash
python3 tools/check_profiles.py
```

---

## 11) Wrapper script summary (`build.ps1` and `build.sh`)

- Root wrappers (`/build.sh`, `/build.ps1`) are the stable commands users should run.
- `tools/build.sh` and `tools/build.ps1` are compatibility shims.
- New build/run feature behavior must be implemented in `tools/build.py`.

# Userspace runtime model

YAML targets may select an independent root userspace policy with
`userspace.runtime_model`: `direct`, `static`, `light`, or `full`. The target
resolver defaults omitted values to `full` during migration. It passes the
resolved `BHARAT_USERSPACE_RUNTIME_MODEL` to CMake and packages exactly one root:
`user_smoke`, `rt-supervisor`, `init-lite`, or `init`, respectively. Unknown
values and the unsupported top-level `runtime_model` spelling fail validation.

# Native syscall ABI reproducibility

Verify the Native manifest, compatibility lock, and deterministic generated
numbers/table artifacts without writing into the source tree:

```bash
python3 tools/abi/syscall_abi.py --check
python3 -m unittest tools.abi.test_syscall_abi
```

Intentional, reviewed Native syscall additions require an explicit
`python3 tools/abi/syscall_abi.py --update-lock`; see
`docs/dev/native-syscall-abi-change.md` for the full procedure.

# Runtime implementation maturity gate

Every configure/build action checks `interface/contracts/implementation_maturity.json` before the
linker runs. Targets declare `implementation_maturity.profile`; release and hardened profiles reject
`STUB` and `TEST_ONLY` implementations. A development
target may explicitly opt in with `implementation_maturity.allow: [STUB]` (and/or `TEST_ONLY`) in its
target YAML. The focused standalone check is:

```bash
python3 tools/check_implementation_maturity.py --profile RELEASE
```

## Architecture dependency linting

The layer-reference and CMake target-dependency gates apply the repository's
checked-in technical-debt baselines by default, so the standard commands fail
only for new architecture-boundary regressions:

```bash
python3 tools/lint/check_layer_references.py --strict
python3 tools/lint/check_cmake_dependencies.py --strict
```

Use `--no-baseline` for an explicit audit of all known debt. A custom baseline
can still be selected with `--baseline <path>`.

# HMEM/Tensor QEMU benchmarks

The release-style HMEM benchmark profiles enable benchmark telemetry while
keeping it disabled in normal builds. Run one certified 64-bit QEMU target or
the three-target matrix with:

```bash
python3 tools/bench/run_hmem_bench.py --target delivery/targets/qemu/x86_64_hmem_bench_release.yaml
python3 tools/bench/run_hmem_bench.py --matrix
```

Results are written as JSON and CSV below `build/bench-results`. Deterministic
copy/allocation/checksum metrics are validation gates. QEMU elapsed time is only
a software-overhead indicator, not real GPU/NPU/DMA performance evidence. See
`quality/benchmarks/README.md` for the evidence and claim boundary.
