# Bharat-OS Diagnostics Application (`bharat-info`)

This is a native userspace diagnostic application designed to demonstrate working Bharat-OS APIs. It reports data available through natively exposed userspace interfaces.

## Features

- **Monotonic Uptime**: Retrieves and displays the system's monotonic uptime using `bh_time_get(BH_CLOCK_MONOTONIC)`.
- **Memory Smoke Test**: Performs a memory allocation via `bh_alloc_ex()` to confirm userspace virtual memory capabilities. Memory unmapping is currently not exposed in the `bh_native.h` API and is marked as unsupported.
- **Unsupported Information**: Explicitly identifies missing OS versioning and process identity endpoints due to the lack of necessary generic syscall/IPC interfaces natively available in the runtime.

## Build Instructions

1. Configure and build the target architecture (e.g., `x86_64_desktop_headless`):
   ```bash
   ./nirmaan build delivery/targets/qemu/x86_64_desktop_headless.yaml --mode release
   ```

2. The application binary will be output to:
   ```
   build/x86_64-qemu-desktop-release/bharat_user/apps/bharat-info/app_bharat_info
   ```

## Execution in QEMU

You can execute the built environment (which packages this binary into the initrd if configured as a startup service) using `nirmaan`:

```bash
./nirmaan run delivery/targets/qemu/x86_64_desktop_headless.yaml --mode release
```

*Note: You may need to wire this application to be executed on boot by adding it to the respective system launch manifest if you want it to run automatically, or invoke it via the system shell.*
