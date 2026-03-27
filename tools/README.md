# Bharat-OS Tools Directory

This directory contains the essential build, test, and emulation scripts for Bharat-OS development.

## Core Tooling
- `build.sh` / `build.ps1`: The primary entry points for building the entire kernel and launching it in QEMU.
- `debug.sh` / `debug.ps1`: Scripts to attach a debugger (GDB/LLDB) to running QEMU instances.
- `sign_release.py`: Handles payload and image signing.

## Usage Overview
To configure, compile, and optionally run the kernel for a specific architecture:

```bash
# Linux / macOS
./build.sh x86_64 --run

# Windows (PowerShell)
.\build.ps1 -Arch x86_64 -Run
```

For advanced usage (like profiling, hardware targeting, or dual serial output), use `--help` or see the corresponding scripts.

## Relationship to SDK
The `tools/` directory is concerned with building the *kernel* and running the *emulation environment*. To build user-space applications, look inside `user/sdk/`.
