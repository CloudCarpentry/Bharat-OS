# Kernel Generated Configuration

This directory formerly contained committed `bharat_config.h` artifacts. To prevent stale values, noisy diffs, and merge conflicts, **generated code is no longer committed to the source tree.**

## How it works now

1. **Template**: The source of truth for the configuration header is `core/kernel/include/bharat_config.h.in`.
2. **Generation**: CMake processes this template during the configuration phase and outputs the final `bharat_config.h` into the build directory:
   - Path: `${CMAKE_BINARY_DIR}/generated/include/bharat_config.h`
3. **Consumption**: Kernel targets include the build directory's generated include path.

## Why the shim exists?

The `bharat_config.h` file in this directory is a **fail-fast shim**. It is designed to trigger a compilation error if any part of the build system accidentally attempts to use the source-tree path instead of the build-generated path.

## Contributor Rules

- **DO NOT** commit a real `bharat_config.h` to this directory.
- **DO NOT** edit the shim manually to add configuration.
- **DO** edit `core/kernel/include/bharat_config.h.in` if you need to add or change available configuration macros.
