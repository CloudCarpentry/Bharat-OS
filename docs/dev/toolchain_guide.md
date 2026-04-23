# Bharat-OS Toolchain Guide

## Purpose
This guide explains how the Bharat-OS build system handles cross-compilation, toolchain discovery, and configuration across the kernel, drivers, SDK, and user applications.

## Cross-Architecture Support
Bharat-OS leverages CMake toolchain files to decouple host build environments from target hardware architecture.

Supported target architectures:
- `x86_64`
- `arm64`
- `riscv64`

Toolchains are defined in `cmake/toolchains/<arch>-elf.cmake`.

## Host vs. Target Environments
- **Host System:** The machine executing the compiler (e.g., your laptop running Ubuntu or Windows).
- **Target System:** The machine executing the built binary (e.g., a QEMU emulated board or physical device).

The toolchain files ensure that CMake correctly identifies `CMAKE_SYSTEM_NAME` (e.g., "Generic" for bare-metal) and links against correct system roots rather than the host's standard library.

## Common Developer Workflows

### 1. Building the Kernel
```bash
./build.sh build --target x86_64
```
*Behind the scenes: Calls CMake using `cmake/toolchains/x86_64-elf.cmake`.*

### 2. Building the SDK
```bash
cd user/sdk/
./build.sh build --target arm64
```
*Behind the scenes: Leverages the exact same toolchain files to ensure ABI consistency.*

## Environment Validation
The build scripts in `tools/` and `user/sdk/` have been enhanced to perform basic environment validation, verifying that CMake and Ninja/Make exist before proceeding, which prevents cryptic linker errors later.

## Debugging Build Failures
1. Check that the correct `ARCH` is passed.
2. If changing architectures, run the scripts with `--clean` or `-Clean`.
3. If building on Windows natively (not WSL), ensure you are using Clang and have `bharat_user_buildopts` linked appropriately.
