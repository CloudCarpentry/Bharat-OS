# Building Bharat-OS

Bharat-OS uses **LLVM/Clang** and **LLD** exclusively (no GCC/GNU Make), keeping the entire toolchain MIT-permissive. Build scripts work identically on Windows, WSL, Linux, macOS, and BSD.

---

## Prerequisites

| Tool        | Version | Purpose                          |
| ----------- | ------- | -------------------------------- |
| CMake       | ≥ 3.20  | Build system generator           |
| Clang + LLD | ≥ 16    | C compiler + linker (bare-metal) |
| Ninja       | any     | Fast build backend               |
| QEMU        | any     | Hardware emulator for testing    |

### Install on Windows (PowerShell as Administrator)

```powershell
winget install -e --id Kitware.CMake
winget install -e --id LLVM.LLVM
winget install -e --id Ninja-build.Ninja
winget install -e --id SoftwareFreedomConservancy.QEMU
```

> Restart your terminal after installation so tools appear in `PATH`.

### Install on Ubuntu / Debian / WSL

```bash
sudo apt update
sudo apt install -y cmake ninja-build clang lld qemu-system-x86 qemu-system-misc
```

### Install on Arch Linux

```bash
sudo pacman -S cmake ninja clang lld qemu-desktop
```

### Install on macOS (Homebrew)

```bash
brew install cmake ninja llvm qemu
export PATH="$(brew --prefix llvm)/bin:$PATH"   # add to ~/.zshrc
```

### Install on FreeBSD / NetBSD / OpenBSD

```bash
# FreeBSD
pkg install cmake ninja llvm qemu-system-x86_64

# OpenBSD
pkg_add cmake ninja llvm qemu
```

---

## How the Build System Works

All compiler and linker settings are isolated in **CMake toolchain files** under `cmake/toolchains/`.
Setting `CMAKE_SYSTEM_NAME=Generic` in the toolchain file is what prevents CMake from injecting host-OS-specific flags (MSVC import libs on Windows, macOS rpaths, etc.). This is the same pattern used by Zephyr, seL4, and other bare-metal kernel projects.

```
cmake/toolchains/
  x86_64-elf.cmake   ← x86_64 bare-metal (QEMU)
  riscv64-elf.cmake  ← RISC-V 64-bit bare-metal (QEMU)
  arm64-elf.cmake    ← ARM64 bare-metal (compile validation)
```

---

## Building (All Platforms)

### Option A — Convenience scripts (Recommended)

**Linux / macOS / WSL / BSD (bash)**

```bash
chmod +x tools/build.sh

# Build x86_64
./tools/build.sh x86_64

# Build and boot in QEMU
./tools/build.sh x86_64 --run

# Clean build + QEMU
./tools/build.sh x86_64 --clean --run

# Build RISC-V 64-bit
./tools/build.sh riscv64

# Build ARM64 (compile-only scaffold)
./tools/build.sh arm64

# Override boot knobs
./tools/build.sh x86_64 --boot-gui=OFF --hw=vm
```

**Windows (PowerShell 5+ or pwsh)**

```powershell
# Build x86_64
.\tools\build.ps1

# Build and boot in QEMU
.\tools\build.ps1 -Arch x86_64 -Run

# Clean build + QEMU
.\tools\build.ps1 -Arch x86_64 -Clean -Run

# Build RISC-V 64-bit
.\tools\build.ps1 -Arch riscv64

# Build ARM64 (compile-only scaffold)
.\tools\build.ps1 -Arch arm64

# Override boot knobs
.\tools\build.ps1 -Arch x86_64 -BootGui OFF -HardwareProfile vm
```

---

### Option B — Raw CMake commands

Same commands work on every platform:

```bash
# x86_64
cmake -S kernel -B build/x86_64 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64-elf.cmake \
      -G Ninja

cmake --build build/x86_64 --target kernel.elf

# RISC-V 64-bit
cmake -S kernel -B build/riscv64 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv64-elf.cmake \
      -G Ninja

cmake --build build/riscv64 --target kernel.elf
```

On **Windows**, use the same commands from PowerShell — CMake finds Clang automatically from `C:\Program Files\LLVM\bin`. If Clang is not on `PATH`, pass the compiler explicitly:

```powershell
cmake -S kernel -B build/x86_64 `
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64-elf.cmake `
      -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang.exe" `
      -G Ninja
```

---

## Running in QEMU

```bash
# x86_64 — serial output to terminal
qemu-system-x86_64 -kernel build/x86_64/kernel.elf \
    -m 256M -nographic -serial mon:stdio -no-reboot

# RISC-V 64-bit
qemu-system-riscv64 -machine virt \
    -kernel build/riscv64/kernel.elf \
    -m 256M -nographic -serial mon:stdio -no-reboot
```

> Press **Ctrl+A then X** to quit QEMU.

---

## Build Output

| File                      | Description                                 |
| ------------------------- | ------------------------------------------- |
| `build/<arch>/kernel.elf` | Bare-metal ELF image, loadable by GRUB/QEMU |

---

## Supported Architectures

| Arch      | Status                                     | QEMU Machine                        |
| --------- | ------------------------------------------ | ----------------------------------- |
| `x86_64`  | ✅ Active                                  | `qemu-system-x86_64 -kernel`        |
| `riscv64` | ✅ Cross-compile validated (incl. Shakti RV64 profile) | `qemu-system-riscv64 -machine virt` |
| `arm64`   | ✅ Cross-compile validated (runtime pending) | N/A                                 |


---

## Portability Matrix

Bharat-OS exclusively relies on LLVM/Clang and LLD (version 16+) for bare-metal compilation. This strategy ensures cross-compilation stability and avoids conflicts seen with standard C libraries.

| Architecture | Target Triple         | Compiler | Linker | Status      |
| ------------ | --------------------- | -------- | ------ | ----------- |
| `x86_64`     | `x86_64-elf`          | Clang 16+| LLD 16+| Active      |
| `riscv64`    | `riscv64-elf`         | Clang 16+| LLD 16+| Planned     |
| `arm64`      | `aarch64-elf`         | Clang 16+| LLD 16+| Experimental|

---

## Running the AI Governor in User Space

During early bring-up, the AI Governor operates as an isolated user-space process. It uses the capability-based IPC model (specifically the Lockless URPC messaging spine or Synchronous Endpoint IPC) to analyze telemetry from the microkernel and suggest configuration tuning.

To run the AI governor in user space during development or testing:

1. **Build the subsystem:** The governor is located in `subsys/src/ai_governor.c`. Ensure it is built using the same bare-metal toolchain provided in `cmake/toolchains/`. (Note: A testing wrapper can also be built as a standalone binary on the host to simulate telemetry).
2. **Execute integration tests:** Run the tests located in the `tests/` directory (e.g., `test_ai_governor`) to verify IPC message formatting and URPC ring behavior before booting the full kernel image in QEMU.
3. **Boot in Emulator:** Once built into the root filesystem image (pending storage subsystem availability), the microkernel will spawn the AI governor as a capability-restricted task upon boot.


---

## Troubleshooting

**`clang not found` (CMake error)**
→ Install LLVM (see above). On macOS, also run `export PATH="$(brew --prefix llvm)/bin:$PATH"`.

**`ld.lld not found`**
→ Install `lld`. On Ubuntu: `sudo apt install lld`. On macOS: included with Homebrew LLVM.

**QEMU: `-nographic` freezes**
→ Press Ctrl+A then X to exit. Or use `-serial stdio` without `-nographic` to open a separate window.

**Windows: `ninja` not found by CMake**
→ Install Ninja via winget (see above) or set path: `$env:Path += ";C:\path\to\ninja"`.


## Boot-time configuration

You can tune early boot behavior without source edits:

- `BHARAT_BOOT_GUI` (`ON`/`OFF`): enables boot-to-GUI handoff metadata.
- `BHARAT_BOOT_HW_PROFILE` (`generic`, `desktop`, `server`, `vm`, `laptop`): picks hardware profile compile definitions for boot policy and defaults.

These are wired through both build scripts and raw CMake cache entries.
