# Building for ARM64

The `arm64` architecture is currently a **Cross-compile validated (runtime pending)** target for Bharat-OS. It uses a bare-metal toolchain based on `aarch64-elf-clang` and `aarch64-elf-lld`. This document outlines how to build and validate the ARM64 compile process.

## Prerequisites

Before starting, ensure your environment is configured according to the [Environment Preparation Guide](ENV_PREP.md).

## Build Options

The build system offers multiple paths for configuring and building the kernel. The options below are supported across Linux, Windows, macOS, WSL, and BSD.

### Using Convenience Scripts (Recommended)

**Linux / macOS / WSL / BSD:**

```bash
chmod +x ./build.sh

# Build the kernel
./build.sh build --target arm64

# Clean and rebuild the kernel
./build.sh build --target arm64

# Build and attempt to boot in QEMU (Runtime pending)
./build.sh all --target arm64
```

**Windows (PowerShell):**

```powershell
# Build the kernel
.\build.ps1 build --target arm64

# Clean and rebuild the kernel
.\build.ps1 build --target arm64

# Build and attempt to boot in QEMU (Runtime pending)
.\build.ps1 all --target arm64
```

### PowerShell ↔ Bash Argument Mapping (ARM64)

If you run this on Windows:

```powershell
.\build.ps1 all --target arm64_desktop_gui
```

the equivalent command on Linux/macOS/WSL/BSD is:

```bash
./build.sh all --target arm64_desktop_gui
```

Short-form flags also work in `build.sh`, so this is equivalent too:

```bash
./build.sh all --target arm64_desktop_gui
```

### Using CMake Presets

You can use standard CMake presets for compiling the kernel and executing tests.

```bash
# Configure and build the kernel
cmake --preset arm64-elf-debug
cmake --build --preset build-arm64-kernel
```

### Using Raw CMake Commands

For granular control, you can manually configure and build using CMake:

```bash
cmake -S . -B build/arm64 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm64-elf.cmake \
      -G Ninja

cmake --build build/arm64 --target kernel.elf
```

## Running the Kernel in QEMU

While runtime execution is currently pending full validation on `arm64`, you can still test the generated ELF file in QEMU for bootloader and early initialization issues.

```bash
# Serial output to terminal
qemu-system-aarch64 -machine virt -cpu cortex-a57 \
    -kernel build/arm64/kernel.elf \
    -m 256M -nographic -serial stdio -no-reboot
```

> **Note:** To exit QEMU, press `Ctrl+A` followed by `X`. To use a separate QEMU window, remove `-nographic` and use `-serial stdio`.

## Testing

Host-side testing functions identically for ARM64 code pathways. These tests run on your local machine and bypass QEMU entirely.

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

Support for building a standalone SDK for the `arm64` architecture, including headers and pre-compiled libraries for user-space development, is planned. Future updates will detail SDK packaging commands and integration with external user-space projects.
