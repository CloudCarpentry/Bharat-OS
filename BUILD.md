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

## 1) Tool entrypoints and command model

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
.\build.ps1 build --target x86_64_desktop_headless
.\build.ps1 all --target x86_64_desktop_headless
.\build.ps1 run --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml
```

```bash
# Linux/macOS/WSL
./build.sh build --target x86_64_desktop_headless
./build.sh all --target x86_64_desktop_headless
./build.sh run --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml
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
- (optional) `opensbi` for RISC-V environments
- (optional) `openocd` for board flashing
- (optional) `gdb-multiarch`

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

---

## 5) Preset command cookbook

## 5.1 Canonical headless run commands (PowerShell)

```powershell
.\build.ps1 all --target x86_64_desktop_headless
.\build.ps1 all --target arm64_desktop_headless
.\build.ps1 all --target riscv64_desktop_headless
.\build.ps1 all --target arm32_mmu_lite_headless
.\build.ps1 all --target riscv32_mmu_lite_headless

.\build.ps1 all --target x86_64_desktop_headless_gp
.\build.ps1 all --target x86_64_desktop_headless_rt
.\build.ps1 all --target x86_64_desktop_headless_mix

.\build.ps1 all --target arm64_desktop_headless_gp
.\build.ps1 all --target arm64_desktop_headless_rt
.\build.ps1 all --target arm64_desktop_headless_mix

.\build.ps1 all --target riscv64_desktop_headless_gp
.\build.ps1 all --target riscv64_desktop_headless_rt
.\build.ps1 all --target riscv64_desktop_headless_mix

.\build.ps1 all --target arm32_mmu_lite_headless_gp
.\build.ps1 all --target arm32_mmu_lite_headless_rt
.\build.ps1 all --target arm32_mmu_lite_headless_mix

.\build.ps1 all --target riscv32_mmu_lite_headless_gp
.\build.ps1 all --target riscv32_mmu_lite_headless_rt
.\build.ps1 all --target riscv32_mmu_lite_headless_mix

.\build.ps1 all --target x86_64_gp_headless
.\build.ps1 all --target x86_64_rt_headless
.\build.ps1 all --target x86_64_mix_headless
.\build.ps1 all --target arm64_gp_headless
.\build.ps1 all --target arm64_rt_headless
.\build.ps1 all --target arm64_mix_headless
.\build.ps1 all --target riscv64_gp_headless
.\build.ps1 all --target riscv64_rt_headless
.\build.ps1 all --target riscv64_mix_headless
.\build.ps1 all --target arm32_gp_headless
.\build.ps1 all --target arm32_rt_headless
.\build.ps1 all --target arm32_mix_headless
.\build.ps1 all --target riscv32_gp_headless
.\build.ps1 all --target riscv32_rt_headless
.\build.ps1 all --target riscv32_mix_headless
```

## 5.2 Canonical headless run commands (WSL/Linux/macOS)

### Personality Targets (Linux & Android)
You can explicitly build the desktop GP profile with a specific personality:
```bash
./build.sh all --target x86_64_desktop_headless_linux
./build.sh all --target x86_64_desktop_headless_android
```
These test targets assert that the ABI boundaries and dispatch tables do not cause build breakage or kernel panics during early boot.


```bash
./build.sh all --target x86_64_desktop_headless
./build.sh all --target arm64_desktop_headless
./build.sh all --target riscv64_desktop_headless
./build.sh all --target x86_64_desktop_headless_linux
./build.sh all --target arm64_desktop_headless_linux
./build.sh all --target riscv64_desktop_headless_linux
./build.sh all --target x86_64_desktop_headless_android
./build.sh all --target arm64_desktop_headless_android
./build.sh all --target riscv64_desktop_headless_android
./build.sh all --target arm32_mmu_lite_headless
./build.sh all --target riscv32_mmu_lite_headless

./build.sh all --target x86_64_desktop_headless_gp
./build.sh all --target x86_64_desktop_headless_rt
./build.sh all --target x86_64_desktop_headless_mix

./build.sh all --target arm64_desktop_headless_gp
./build.sh all --target arm64_desktop_headless_rt
./build.sh all --target arm64_desktop_headless_mix

./build.sh all --target riscv64_desktop_headless_gp
./build.sh all --target riscv64_desktop_headless_rt
./build.sh all --target riscv64_desktop_headless_mix

./build.sh all --target arm32_mmu_lite_headless_gp
./build.sh all --target arm32_mmu_lite_headless_rt
./build.sh all --target arm32_mmu_lite_headless_mix

./build.sh all --target riscv32_mmu_lite_headless_gp
./build.sh all --target riscv32_mmu_lite_headless_rt
./build.sh all --target riscv32_mmu_lite_headless_mix

./build.sh all --target x86_64_gp_headless
./build.sh all --target x86_64_rt_headless
./build.sh all --target x86_64_mix_headless
./build.sh all --target arm64_gp_headless
./build.sh all --target arm64_rt_headless
./build.sh all --target arm64_mix_headless
./build.sh all --target riscv64_gp_headless
./build.sh all --target riscv64_rt_headless
./build.sh all --target riscv64_mix_headless
./build.sh all --target arm32_gp_headless
./build.sh all --target arm32_rt_headless
./build.sh all --target arm32_mix_headless
./build.sh all --target riscv32_gp_headless
./build.sh all --target riscv32_rt_headless
./build.sh all --target riscv32_mix_headless
```

## 5.3 GUI presets (examples)

```powershell
.\build.ps1 all --target x86_64_desktop_gui
.\build.ps1 all --target arm64_desktop_gui
.\build.ps1 all --target riscv64_desktop_gui
```

```bash
./build.sh all --target x86_64_desktop_gui
./build.sh all --target arm64_desktop_gui
./build.sh all --target riscv64_desktop_gui
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
./build.sh build --target x86_64_desktop_headless
./build.sh package --target x86_64_desktop_headless
./build.sh run --target x86_64_desktop_headless
```

YAML target path equivalent:

```bash
./build.sh all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml
```

---

## 7) Debug workflow

Current state:

- `build.py debug` exists in CLI.
- The main debug stage prints `Debug workflow not fully implemented yet.`
- You can still use helper debugger attach scripts:
  - `tools/debug.sh`
  - `tools/debug.ps1`

Typical pattern:

1. launch QEMU with gdb stub enabled from target extra args (if configured),
2. attach with debug helper script.

---

## 8) Board and hardware flashing workflow

For hardware/board flows, use `flash` with a target that includes flash configuration.

```powershell
.\build.ps1 flash --target <board_target>
.\build.ps1 flash --target <board_target> --dry-run
```

```bash
./build.sh flash --target <board_target>
./build.sh flash --target <board_target> --dry-run
```

Flash backend currently implemented: **OpenOCD**.

Install OpenOCD first (`openocd` command in PATH).

---

## 9) Discovering available presets and targets

List CLI usage:

```bash
./build.sh --help
./build.sh build --help
```

Print all legacy target names from `build_config.json`:

```bash
python3 - <<'PY'
import json
cfg = json.load(open('build_config.json'))['builds']
for name in cfg:
    print(name)
PY
```

See YAML targets under:

- `delivery/targets/qemu/` (preferred)
- `tools/targets/qemu/` (legacy alias accepted by `tools/build.py` during migration)

---

## 10) Wrapper script summary (`build.ps1` and `build.sh`)

- Root wrappers (`/build.sh`, `/build.ps1`) are the stable commands users should run.
- `tools/build.sh` and `tools/build.ps1` are compatibility shims.
- New build/run feature behavior must be implemented in `tools/build.py`.
