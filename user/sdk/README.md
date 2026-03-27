# Bharat-OS SDK Directory

This folder contains the software development kit (SDK) used to build user-space applications and services for Bharat-OS.

## What's in here?
- `lib/`: Contains the core system call implementations and primitive libraries (e.g., `syscalls.c`).
- `sample_app/`: A template application demonstrating how to link against the SDK.
- `build.sh` / `build.ps1`: Automated build scripts for Linux/macOS and Windows, respectively.

## How to use it
To build the SDK and sample application:

```bash
# On Linux / macOS
./build.sh --arch x86_64 --clean

# On Windows PowerShell
.\build.ps1 -Arch x86_64 -Clean
```

## How it relates to the workflow
Applications built here link against `libbharat_sdk.a`. The outputs are strictly user-space binaries intended to be run under the Bharat-OS kernel execution environment.

For detailed guides, please see:
- `docs/dev/sdk_guide.md`
- `docs/dev/toolchain_guide.md`
