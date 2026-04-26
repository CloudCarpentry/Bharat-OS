# Bharat-OS Kernel Configuration Agent Instructions

## Configuration System

The kernel configuration is managed via a template-to-header generation process.

- **Source Template**: `core/kernel/include/bharat_config.h.in`
- **Generated Header**: `build/<target>/generated/include/bharat_config.h`

### Guidelines for Agents

1. **Do Not Commit Generated Headers**: Never commit a generated `bharat_config.h` to the source tree. The directory `core/kernel/include/generated/` contains a fail-fast shim that must remain unchanged unless the shim logic itself needs updating.
2. **Modify the Template**: If you need to add new compile-time configuration constants, add them to `core/kernel/include/bharat_config.h.in` using CMake `@VAR@` or `#cmakedefine` syntax.
3. **Build Directory Only**: All generated artifacts must reside in the build directory (`${CMAKE_BINARY_DIR}`).
4. **Include Resolution**: Always use `#include "bharat_config.h"`. The build system is configured to prioritize the build-generated header.
5. **Validation**: When refactoring or adding profiles, ensure that the configuration is deterministic and that runtime hardware discovery (HAL) is used for dynamic facts instead of hardcoded macros where possible.

### Build Verification

After changing configuration templates, verify by building multiple profiles:
```bash
./build.sh all --target x86_64_desktop_headless
./build.sh all --target arm64_desktop_headless
./build.sh all --target riscv64_desktop_headless
```
