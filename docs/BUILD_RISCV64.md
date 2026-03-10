# Building for RISC-V 64-bit

The `riscv64` architecture is currently a **Cross-compile validated** target for Bharat-OS. It uses an LLVM/Clang and LLD toolchain. Additionally, an optional GCC flow is provided for packaging OpenSBI firmware for Shakti RISC-V boards. This document outlines how to build, test, and run the RISC-V 64-bit target.

## Prerequisites

Before starting, ensure your environment is configured according to the [Environment Preparation Guide](ENV_PREP.md).

If you are targeting Shakti boards, you will need the optional Shakti RISC-V toolchain. See the [Shakti RISC-V Toolchain (Optional)](ENV_PREP.md#shakti-risc-v-toolchain-optional) section for installation instructions.

## Build Options

The build system offers multiple paths for configuring and building the kernel. The options below are supported across Linux, Windows, macOS, WSL, and BSD.

### Using Convenience Scripts (Recommended)

**Linux / macOS / WSL / BSD:**

```bash
chmod +x tools/build.sh

# Build the kernel
./tools/build.sh riscv64

# Clean and rebuild the kernel
./tools/build.sh riscv64 --clean

# Build and boot directly in QEMU
./tools/build.sh riscv64 --run

# Run with a specific QEMU machine (e.g., sifive_u)
./tools/build.sh riscv64 --machine=sifive_u --run

# Build RISC-V GCC OpenSBI payload
./tools/build.sh riscv64 --payload
```

**Windows (PowerShell):**

```powershell
# Build the kernel
.\tools\build.ps1 -Arch riscv64

# Clean and rebuild the kernel
.\tools\build.ps1 -Arch riscv64 -Clean

# Build and boot directly in QEMU
.\tools\build.ps1 -Arch riscv64 -Run

# Run with a specific QEMU machine (e.g., sifive_u)
.\tools\build.ps1 -Arch riscv64 -Machine sifive_u -Run

# Build RISC-V GCC OpenSBI payload
.\tools\build.ps1 -Arch riscv64 -Payload
```

### Using CMake Presets

You can use standard CMake presets for compiling the kernel and executing tests. This is particularly useful for configuring Shakti-specific builds.

```bash
# Configure and build the default kernel
cmake --preset riscv64-elf-debug
cmake --build --preset build-riscv64-kernel

# Configure and build Shakti-focused profiles (Clang/LLD)
cmake --preset riscv64-shakti-e-debug && cmake --build --preset build-riscv64-shakti-e-kernel
cmake --preset riscv64-shakti-c-debug && cmake --build --preset build-riscv64-shakti-c-kernel
cmake --preset riscv64-shakti-i-debug && cmake --build --preset build-riscv64-shakti-i-kernel

# Configure and build Shakti/OpenSBI-friendly GCC presets (Produces payload.bin)
cmake --preset riscv64-elf-gcc-debug && cmake --build --preset build-riscv64-gcc-kernel
cmake --preset riscv64-shakti-e-gcc-debug && cmake --build --preset build-riscv64-shakti-e-gcc-kernel
```

### Using Raw CMake Commands

For granular control, you can manually configure and build using CMake:

```bash
cmake -S . -B build/riscv64 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv64-elf.cmake \
      -G Ninja

cmake --build build/riscv64 --target kernel.elf
```

## Running the Kernel in QEMU

If you are not using the `--run` flag with the build scripts, you can run the generated ELF file directly in QEMU.

```bash
# Serial output to terminal
qemu-system-riscv64 -machine virt \
    -kernel build/riscv64/kernel.elf \
    -m 256M -nographic -serial mon:stdio -no-reboot
```

> **Note:** To exit QEMU, press `Ctrl+A` followed by `X`. To use a separate QEMU window, remove `-nographic` and use `-serial stdio`.

## OpenSBI Packaging (Shakti Boards)

To package the `payload.bin` output for OpenSBI:

1. Build the payload artifacts using the GCC presets or the `--payload` flag with the build script. This will output `kernel.elf` and `kernel.payload.bin`.
2. Package the payload with OpenSBI (replace `/workspace/Bharat-OS` with the path to your Bharat-OS repository):

```bash
# In your OpenSBI checkout
make PLATFORM=generic FW_PAYLOAD_PATH=/workspace/Bharat-OS/build-riscv64-shakti-e-gcc/kernel/kernel.elf
```

At runtime, OpenSBI will pass the `(hartid, fdt_ptr)` to the Bharat-OS `_start` / `kernel_main` entry points.

## Testing

Host-side testing works similarly across architectures, running natively on your machine to validate the core logic, profile integration, and system behaviors.

**Building and running host-side tests:**

```bash
# Configure and build the host-side test suite
cmake --preset tests-host
cmake --build --preset build-tests

# Run all tests natively
ctest --preset run-tests
```

### Target-Specific Testing

Target-specific tests are currently under development. To validate architecture-specific bindings, rely on the runtime execution tests via QEMU using the `--run` parameter during the build process.

## SDK Development (Planned)

Support for building a standalone SDK for the `riscv64` architecture, including headers and pre-compiled libraries for user-space development, is planned. Future updates will detail SDK packaging commands and integration with external user-space projects.
