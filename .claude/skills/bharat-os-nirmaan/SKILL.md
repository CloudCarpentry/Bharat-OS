---
name: bharat-os-nirmaan
description: Canonical skill for using the Nirmaan Developer CLI. Encourages agents and developers to use nirmaan for all building, compiling, smoke testing, running, interactive sessions, board verification, packaging, and demo workflows across Bharat-OS.
---

# Bharat-OS Nirmaan Developer CLI Skill

The `nirmaan` CLI is the canonical, high-productivity interface for all Bharat-OS development, build, test, verification, and execution workflows.

**Golden Rule for Autonomous & Interactive Agents**:
> **Always prefer `nirmaan` over invoking ad-hoc CMake commands, raw build flags, or manual emulator invocations.**  
> Using Nirmaan guarantees that target composition, CMake presets, kernel component policies, runtime models, and packaging manifests are resolved deterministically.

---

## 1. Invocation by Environment

Nirmaan provides platform-native entrypoints that all forward to `tools/nirmaan/cli.py`:

| Platform | Recommended Invocation | Fallback Invocation |
|---|---|---|
| **Windows (PowerShell)** | `.\nirmaan.ps1 <command> [args]` | `python tools/nirmaan/cli.py <command> [args]` |
| **Windows (cmd.exe)** | `nirmaan.bat <command> [args]` | `python tools/nirmaan/cli.py <command> [args]` |
| **Linux / WSL / macOS** | `./nirmaan <command> [args]` | `python3 tools/nirmaan/cli.py <command> [args]` |

---

## 2. Core Command Reference

### Environment & Diagnostics
```bash
# Check development environment (Clang, LLD, QEMU matrix, CMake, Ninja, SDK)
nirmaan doctor

# List available shortcut targets and their architectural profiles
nirmaan targets
```

### Building & Compiling
```bash
# Compile a target (default: build action only)
nirmaan build desktop-x86_64
nirmaan build controller-arm32
nirmaan build stm32f4-renode

# Compile with explicit build mode
nirmaan build desktop-arm64 --mode release
nirmaan build desktop-riscv64 --mode development

# Compile using explicit target YAML
nirmaan build --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml

# Compile only without running (explicit compile guard)
nirmaan build stm32f4-renode --build-only
```

### Smoke Build & Interactive Execution
```bash
# Smoke build: compile, package, and execute automated headless smoke verification
nirmaan build desktop-x86_64 --smoke
nirmaan build desktop-arm64 --smoke
nirmaan build --target-yaml delivery/targets/qemu/riscv32_mmu_lite_headless.yaml --smoke

# Interactive build: compile, package, and launch an interactive console session
nirmaan build desktop-x86_64 --interactive

# Direct target execution (from existing build/package outputs)
nirmaan run desktop-x86_64 --smoke
nirmaan run desktop-x86_64 --interactive
nirmaan run stm32f4-renode
```

### Testing & Verification
```bash
# Smoke-test individual architecture targets
nirmaan test desktop-x86_64
nirmaan test desktop-arm64
nirmaan test desktop-riscv64
nirmaan test controller-arm32

# Embedded board qualification & unit tests
nirmaan test board
# Qualify all 4 embedded board manifests (MMIO collision, flash/SRAM fit, stack room)
nirmaan verify --all-boards
# Qualify a specific board
nirmaan verify --board stm32f407_discovery
nirmaan verify --board hifive1_rev_b
```

### End-to-End Demonstrations
```bash
# Build, package, and execute end-to-end multi-core demonstrations
nirmaan demo x86_64_hmem_demo
nirmaan demo arm64_hmem_demo
nirmaan demo riscv64_hmem_demo
```

### Packaging
```bash
# Package target ELF binaries, boot assets, and run-manifest.json
nirmaan package desktop-x86_64
nirmaan package stm32f4-renode
```

---

## 3. Standard Target Shortcuts

Nirmaan maps high-level target names to their concrete target YAML configurations:

| Shortcut | Architecture | Profile | Memory Model | Display | Output Path |
|---|---|---|---|---|---|
| `desktop-x86_64` | `x86_64` | Desktop | `MMU_FULL` | GUI / Headless | `build/x86_64-desktop-debug/` |
| `desktop-arm64` | `arm64` | Desktop | `MMU_FULL` | GUI / Headless | `build/arm64-desktop-debug/` |
| `desktop-riscv64` | `riscv64` | Desktop | `MMU_FULL` | GUI / Headless | `build/riscv64-desktop-debug/` |
| `controller-arm32` | `arm32` | Controller | `MMU_LITE` / `MPU`| Headless | `build/arm32-mmu-lite-debug/` |
| `stm32f4-renode` | `arm32` | IoT / MCU | `MPU_ONLY` | Headless (Renode)| `build/arm32-iot-debug/` |
| `drone-arm64` | `arm64` | Robotics | `MMU_FULL` | Headless | `build/arm64-robotics-debug/` |
| `robot-riscv64` | `riscv64` | Robotics | `MMU_FULL` | GUI | `build/riscv64-robotics-debug/` |

---

## 4. Agent Best Practices & Workflow Guidelines

1. **Verify Environment First**: When starting work on a new machine or fresh container, always run `nirmaan doctor` to identify any missing toolchains or emulators before attempting builds.
2. **Use Smoke Mode for Fast Feedback**: When testing a feature or bugfix, use `nirmaan build <target> --smoke` to compile and verify execution in a single command.
3. **Use `test board` for Embedded Changes**: Any change modifying `platform/boards/`, `delivery/linker/`, or embedded memory models must be validated with `nirmaan verify --all-boards` and `nirmaan test board`.
4. **Interactive Debugging**: If a smoke test fails or an agent needs to inspect runtime shell output, use `nirmaan build <target> --interactive` or `nirmaan run <target> --interactive`.
5. **Demos**: Before documenting demonstration capabilities, always confirm they execute via `nirmaan demo <demo_name>`.
