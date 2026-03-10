# Environment Preparation

This guide covers how to set up your development environment for building Bharat-OS across various platforms.

Bharat-OS primarily uses **LLVM/Clang** + **LLD** for cross-architecture reproducibility, with an optional **riscv64-unknown-elf-gcc** flow for Shakti/OpenSBI firmware packaging. The build scripts work on Windows, WSL, Linux, macOS, and BSD.

## Prerequisites

| Tool | Version | Purpose |
| --- | --- | --- |
| CMake | ≥ 3.20 | Build system generator |
| Clang + LLD | ≥ 16 | C compiler + linker (bare-metal) |
| Ninja | any | Fast build backend |
| QEMU | any | Hardware emulator for testing |
| Python | ≥ 3.x | Build scripts, utilities |
| GDB | any | Debugger |
| GCC / Binutils | any | Optional toolchain (RISC-V/OpenSBI, tests) |

---

## Coding Agent Environment (Jules, Codex, etc.)

For coding agents and automated CI/CD environments, you can use the following script to install all necessary tools on a Debian/Ubuntu-based system (like WSL or a Linux container):

```bash
#!/usr/bin/env bash
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive

sudo apt-get update
sudo apt-get install -y \
  git bash curl wget ca-certificates \
  make cmake ninja-build pkg-config \
  clang lld llvm gcc g++ binutils nasm \
  gdb gdb-multiarch \
  python3 python3-pip \
  qemu-system-x86 qemu-system-arm qemu-system-misc \
  ovmf mtools dosfstools xorriso \
  grub-pc-bin grub-efi-amd64-bin

cmake --version
ninja --version
clang --version
ld.lld --version
python3 --version
qemu-system-x86_64 --version
qemu-system-aarch64 --version || true
qemu-system-riscv64 --version || true
```

---

## Platform-Specific Setup

### Windows 11 + WSL (Recommended Flow)

The recommended setup for Windows users is to install tools natively for PowerShell, and also set up a WSL environment for Linux-based testing.

**1. Install on Windows (PowerShell as Administrator)**

```powershell
winget install -e --id Kitware.CMake
winget install -e --id LLVM.LLVM
winget install -e --id Ninja-build.Ninja
winget install -e --id SoftwareFreedomConservancy.QEMU
winget install -e --id Python.Python.3.11
```

> **Note:** Restart your terminal after installation so tools appear in `PATH`. On Windows, `tools/build.ps1` expects QEMU at `C:\Program Files\qemu\` and auto-adds `C:\Program Files\LLVM\bin` to `PATH`.

**2. Install on WSL (Ubuntu/Debian inside Windows)**

Open your WSL terminal and run:

```bash
sudo apt update
sudo apt install -y cmake ninja-build clang lld qemu-system-x86 qemu-system-misc \
  python3 gcc g++ binutils nasm gdb gdb-multiarch ovmf mtools dosfstools xorriso \
  grub-pc-bin grub-efi-amd64-bin
```

### Ubuntu / Debian (Native)

```bash
sudo apt update
sudo apt install -y cmake ninja-build clang lld qemu-system-x86 qemu-system-misc \
  python3 gcc g++ binutils nasm gdb gdb-multiarch ovmf mtools dosfstools xorriso \
  grub-pc-bin grub-efi-amd64-bin
```

### Arch Linux

```bash
sudo pacman -S cmake ninja clang lld qemu-desktop python gcc binutils nasm gdb \
  mtools dosfstools xorriso grub
```

### macOS (Homebrew)

```bash
brew install cmake ninja llvm qemu python gcc binutils nasm gdb mtools dosfstools xorriso
export PATH="$(brew --prefix llvm)/bin:$PATH"   # add to ~/.zshrc
```

### FreeBSD / NetBSD / OpenBSD

**FreeBSD:**
```bash
pkg install cmake ninja llvm qemu-system-x86_64 python gcc binutils nasm gdb
```

**OpenBSD:**
```bash
pkg_add cmake ninja llvm qemu python gcc binutils nasm gdb
```

---

## Shakti RISC-V Toolchain (Optional)

For Shakti boards, install the Shakti RISC-V toolchain (recommended prebuilt installer) and expose binaries in `PATH`:

```bash
# example: toolchain paths from shakti-tools installer
export PATH=$PATH:/opt/shakti/riscv64/bin:/opt/shakti/riscv64/riscv64-unknown-elf/bin
which riscv64-unknown-elf-gcc
which riscv64-unknown-elf-objcopy
```

---

## Next Steps

Once your environment is set up, proceed to the architecture-specific build guides:

- [Building for x86_64](BUILD_X86_64.md)
- [Building for RISC-V 64-bit](BUILD_RISCV64.md)
- [Building for ARM64](BUILD_ARM64.md)
- [General Build Overview](../BUILD.md)
