# Building for x86_64

The `x86_64` architecture is currently an **Active** target for Bharat-OS. It uses a bare-metal toolchain based on `x86_64-elf-clang` and `x86_64-elf-lld`. This document outlines how to build, test, and run the x86_64 target.

## Prerequisites

Before starting, ensure your environment is configured according to the [Environment Preparation Guide](ENV_PREP.md).

## Build Options

The build system offers multiple paths for configuring and building the kernel. The options below are supported across Linux, Windows, macOS, WSL, and BSD.

### Using Convenience Scripts (Recommended)

**Linux / macOS / WSL / BSD:**

```bash
chmod +x tools/build.sh

# Build the kernel
./tools/build.sh x86_64

# Clean and rebuild the kernel
./tools/build.sh x86_64 --clean

# Build and boot directly in QEMU
./tools/build.sh x86_64 --run

# Run with GDB debug server enabled
./tools/build.sh x86_64 --run --debug

# Override boot knobs (e.g., disable GUI, use VM profile)
./tools/build.sh x86_64 --boot-gui=OFF --hw=vm
```

**Windows (PowerShell):**

```powershell
# Build the kernel
.\tools\build.ps1 -Arch x86_64

# Clean and rebuild the kernel
.\tools\build.ps1 -Arch x86_64 -Clean

# Build and boot directly in QEMU
.\tools\build.ps1 -Arch x86_64 -Run

# Run with GDB debug server enabled
.\tools\build.ps1 -Arch x86_64 -Run -DebugQemu

# Override boot knobs
.\tools\build.ps1 -Arch x86_64 -BootGui OFF -HardwareProfile vm
```

### Using CMake Presets

You can use standard CMake presets for compiling the kernel and executing tests.

```bash
# Configure and build the kernel
cmake --preset x86_64-elf-debug
cmake --build --preset build-x86_64-kernel
```

### Using Raw CMake Commands

For granular control, you can manually configure and build using CMake:

```bash
cmake -S . -B build/x86_64 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64-elf.cmake \
      -G Ninja

cmake --build build/x86_64 --target kernel.elf
```

## Running the Kernel in QEMU

If you are not using the `--run` flag with the build scripts, you can run the generated ELF file directly in QEMU.

```bash
# Serial output to terminal
qemu-system-x86_64 -kernel build/x86_64/kernel.elf \
    -m 256M -nographic -serial mon:stdio -no-reboot
```

> **Note:** To exit QEMU, press `Ctrl+A` followed by `X`. To use a separate QEMU window, remove `-nographic` and use `-serial stdio`.

## Testing

Bharat-OS includes host-side tests for core kernel logic, integration behavior, and hardware profiles. These tests execute natively on your host machine (Windows or Linux) bypassing the need for QEMU, making them extremely fast and useful for continuous integration.

**Building and running host-side tests:**

```bash
# Configure and build the host-side test suite
cmake --preset tests-host
cmake --build --preset build-tests

# Run all tests natively
ctest --preset run-tests

# Run specifically the full integration subsystem tests
cd build-tests
ctest -R test_integration_core_subsys -V

# Run specifically the edge/embedded profile tests
ctest -R test_profile_edge -V
```

### Target-Specific Testing

Target-specific tests are currently under development. To validate architecture-specific bindings, rely on the runtime execution tests via QEMU using the `--run` parameter during the build process.

## SDK Development (Planned)

Support for building a standalone SDK for the `x86_64` architecture, including headers and pre-compiled libraries for user-space development, is planned. Future updates will detail SDK packaging commands and integration with external user-space projects.
