# Building Bharat-OS Locally (Cross-Platform)

Since Bharat-OS strictly avoids GNU/GPL components (to maintain an MIT-permissive license structure), we do not use the standard `gcc` (GNU Compiler Collection) or `make` toolchains.

Instead, we use the **LLVM/Clang** ecosystem alongside **CMake**.

## Required Tools

1. **CMake** (v3.20+): The build system generator.
2. **Ninja**: A fast build system (alternative to GNU Make).
3. **Clang / LLVM**: The C compiler and linker used to target x86_64, RISC-V, and ARM64.
4. **QEMU**: The hardware emulator to boot and test the OS.

---

## Installation on Windows (Native)

You can easily install all the required tools on Windows using `winget` (Windows Package Manager) from PowerShell.

Open PowerShell as Administrator and run:

```powershell
# 1. Install CMake
winget install -e --id Kitware.CMake

# 2. Install Ninja Build System
winget install -e --id Ninja-build.Ninja

# 3. Install LLVM / Clang
winget install -e --id LLVM.LLVM

# 4. Install QEMU (for testing the OS)
winget install -e --id SoftwareFreedomConservancy.QEMU
```

_Note: You may need to restart your terminal after installation so that the tools are available in your system `PATH`._

---

## Installation on macOS

Using [Homebrew](https://brew.sh/):

```bash
# Install CMake, Ninja, LLVM, and QEMU
brew install cmake ninja llvm qemu

# Note: macOS Homebrew installs LLVM alongside Apple Clang.
# Make sure to put the brew LLVM in your path or explicitly reference it:
# export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
```

---

## Installation on Linux

### Debian / Ubuntu

```bash
sudo apt update
sudo apt install cmake ninja-build clang llvm lld qemu-system-x86 qemu-system-arm qemu-system-misc
```

### Arch Linux

```bash
sudo pacman -S cmake ninja clang llvm lld qemu-desktop
```

---

## Quick Start with `bosh` (Recommended)

`bosh` is the Bharat-OS shell launcher. It sets the correct environment variables,
cross-compiler prefix and PATH before handing control back to your preferred shell.

### Linux / macOS (bash/zsh)

```bash
# Make the launcher executable (one-time)
chmod +x tools/bosh

# Start an interactive build session
./tools/bosh

# OR run a single build command non-interactively
BHARAT_ARCH=riscv ./tools/bosh cmake --build build/riscv --target kernel.elf
```

### Windows (PowerShell)

```powershell
# Start an interactive build session
.\tools\bosh.ps1

# Build for a specific arch
.\tools\bosh.ps1 -Arch riscv

# Use the module commands directly after importing
Import-Module .\tools\BharatOS.psm1
bharat-build -Arch x86_64
bharat-run   -Arch x86_64
bharat-clean
```

### Available PowerShell Module Commands

| Command                          | Description                        |
| -------------------------------- | ---------------------------------- |
| `bharat-build [-Arch] [-Clean]`  | Configure and compile `kernel.elf` |
| `bharat-run   [-Arch] [-Memory]` | Boot kernel in QEMU                |
| `bharat-clean [-Arch]`           | Remove build artifacts             |

---

Once the tools are installed and present in your `PATH`, open your terminal (PowerShell, Bash, or Zsh), navigate to the `Bharat-OS` project root directory, and run the following commands.

### Step 1: Configure the Build System

Use CMake to generate Ninja build files. We will explicitly tell it to use the `clang` compiler.

**For x86_64:**

```bash
cmake -B build -G Ninja -DCMAKE_C_COMPILER=clang -DARCH=x86_64 -DOS_PROFILE=DESKTOP
```

**For ARM64:**

```bash
cmake -B build -G Ninja -DCMAKE_C_COMPILER=clang -DARCH=arm64 -DOS_PROFILE=DESKTOP
```

**For RISC-V:**

```bash
cmake -B build -G Ninja -DCMAKE_C_COMPILER=clang -DARCH=riscv -DOS_PROFILE=RTOS
```

### Step 2: Compile the Code

Once configured, tell Ninja to execute the build inside the `build/` directory:

```bash
cmake --build build
```

This will produce our standalone kernel executable at:
`build/kernel/kernel.elf`

---

## Testing / Emulating the OS

We built a custom Python script to automatically generate the emulation launch scripts for QEMU.

### Step 1: Generate the Launch Script

Run this script from the workspace root:

```bash
python3 tools/generate_vm.py --arch x86_64 --memory 2048 --cores 4
```

### Step 2: Boot the OS!

This generates a launcher script in your folder (`run_vm_x86_64.bat` on Windows, or `run_vm_x86_64.sh` on macOS/Linux). Just execute it to boot the newly compiled Bharat-OS kernel inside QEMU!

**Windows:**

```powershell
.\run_vm_x86_64.bat
```

**macOS / Linux:**

```bash
./run_vm_x86_64.sh
```
