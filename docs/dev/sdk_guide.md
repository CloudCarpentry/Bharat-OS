# Bharat-OS SDK Guide

## Purpose
The Bharat-OS Software Development Kit (SDK) provides the essential libraries, headers, and build scripts needed to develop user-space applications for Bharat-OS. It abstracts kernel interactions and provides a consistent interface across supported architectures (x86_64, ARM64, RISC-V).

## Supported Workflows
- Building the core SDK static library (`libbharat_sdk.a`).
- Compiling standalone sample applications that link against the SDK.
- Cross-compiling for target hardware.

## Prerequisites
- **Host OS:** Linux, Windows (with WSL or PowerShell), or macOS.
- **Build Tools:** CMake, Ninja or Make.
- **Toolchain:** Clang/LLVM (or GCC where applicable).

## Quick Start: Building a Sample App

### Linux / macOS
```bash
cd user/sdk/
./build.sh --arch x86_64 --clean
```

### Windows (PowerShell)
```powershell
cd user\sdk\
.\build.ps1 -Arch x86_64 -Clean
```

## Output Structure
After building, the artifacts are placed in `user/sdk/build/<arch>/`:
- `libbharat_sdk.a`: The static SDK library.
- `sample_app`: The compiled sample executable.

## Troubleshooting
- **CMake Error: Toolchain file not found:** Ensure you are running the script from within the `user/sdk/` directory.
- **Linker Errors (e.g., `undefined reference to bharat_syscall`):** Verify that your architecture provides a syscall implementation in `user/sdk/lib/src/`.

## Next Steps
To understand the underlying toolchain behavior, see the [Toolchain Guide](toolchain_guide.md).
