---
title: Environment Preparation for Build + Test + Run
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - dev
  - build
see_also:
  - README.md
---
# Environment Preparation for Build + Test + Run

This guide is for the current Bharat-OS build system (`tools/build.py` + wrappers).

## Required tools

| Tool | Minimum | Why |
|---|---:|---|
| Python | 3.x | Build CLI + tooling |
| CMake | 3.20+ | Presets + configure/build/test |
| Ninja | latest | Build backend |
| Clang/LLVM + LLD | 16+ preferred | Cross-compilation and link |
| QEMU | latest | Emulator runtime for `run`/`all` |
| GDB | optional | Debug bring-up |
| OpenOCD | optional | Flash workflows |

## Linux / WSL (Debian/Ubuntu)

```bash
sudo apt update
sudo apt install -y \
  python3 cmake ninja-build \
  clang lld llvm \
  qemu-system-x86 qemu-system-arm qemu-system-misc \
  gdb gdb-multiarch openocd
```

## Windows (PowerShell)

```powershell
winget install -e --id Python.Python.3.11
winget install -e --id Kitware.CMake
winget install -e --id Ninja-build.Ninja
winget install -e --id LLVM.LLVM
winget install -e --id SoftwareFreedomConservancy.QEMU
```

## macOS (Homebrew)

```bash
brew install python cmake ninja llvm qemu gdb openocd
```

If LLVM is not default:

```bash
export PATH="$(brew --prefix llvm)/bin:$PATH"
```

## Quick health checks

```bash
python3 --version
cmake --version
ninja --version
clang --version
ld.lld --version
cmake --list-presets
```

## Next documents

- Build system details: `HOST_BUILD_TEST_RUN_GUIDE.md`
- x86_64 quickstart: `BUILD_X86_64.md`
- arm64 quickstart: `BUILD_ARM64.md`
- riscv64 quickstart: `BUILD_RISCV64.md`
- Build policy roadmap: `HOST_BUILD_TEST_RUN_PLAN.md`
