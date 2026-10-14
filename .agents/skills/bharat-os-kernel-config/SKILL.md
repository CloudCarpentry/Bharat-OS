---
name: bharat-os-kernel-config
description: Apply when changing Bharat-OS configuration templates, CMake variables, target profiles, feature switches, or generated configuration behavior.
---

# Bharat-OS Kernel Configuration Skill

## Configuration Authority

- **Template Source**: `core/kernel/include/bharat_config.h.in`
- **Authoritative CMake Definition**: Root `CMakeLists.txt` and `delivery/targets/`
- **Generated Output**: `${CMAKE_BINARY_DIR}/generated/include/bharat_config.h`

## Invariant Rules

1. **Never Commit Generated Config**:
   - `bharat_config.h` is strictly a build artifact generated under `build/`.
   - Never commit `bharat_config.h` or add manual headers into source trees that duplicate it.
2. **Compile-Time vs. Runtime Discovery**:
   - Only genuinely immutable build-time policy (such as target architecture, profile type, security hardening options) belongs in `bharat_config.h.in`.
   - Dynamic hardware facts (e.g., number of available cores, RAM size, peripheral base addresses) must be discovered at runtime via HAL, device tree, or ACPI, never hardcoded in configuration macros.
3. **Template Syntax Consistency**:
   - Use CMake `@VAR@` or `#cmakedefine` syntax consistently in `bharat_config.h.in`.
4. **Header Inclusion Pattern**:
   - Always include the generated configuration as `#include "bharat_config.h"`.
   - Never use relative paths to reach the generated header.
5. **Memory Model and Profile Matrix**:
   - Any configuration switch affecting memory layout must explicitly evaluate `MMU_FULL`, `MMU_LITE`, and `MPU/PMP` profiles.
   - Unsupported combinations must fail configuration or compile time with an explicit `#error "Unsupported profile/feature combination"`.

## Verification Procedure

1. Modify `core/kernel/include/bharat_config.h.in` or `CMakeLists.txt`.
2. Verify build succeeds across all 5 architectures:
   ```bash
   python3 tools/agents/verify.py --config
   ```
3. Run `python3 tools/lint/check_cmake_dependencies.py`.
4. Ensure no generated files are staged in git status.
