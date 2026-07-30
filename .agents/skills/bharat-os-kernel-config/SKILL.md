---
name: bharat-os-kernel-config
description: Apply when changing Bharat-OS configuration templates, CMake variables, target profiles, feature switches, or generated configuration behavior.
---

# Bharat-OS Kernel Configuration Skill

## Authority

- Template: `core/kernel/include/bharat_config.h.in`
- Generated output: `build/<target>/generated/include/bharat_config.h`

## Procedure

1. Confirm the setting is truly compile-time. Prefer HAL/platform runtime discovery for dynamic hardware facts.
2. Add or modify only the template and authoritative CMake/profile source.
3. Use `@VAR@` or `#cmakedefine` consistently with existing configuration.
4. Keep generation deterministic and build-directory-only.
5. Include the header as `#include "bharat_config.h"`.
6. Do not edit or commit generated headers.
7. Evaluate MMU, MMU-Lite, and MPU implications.
8. Add configuration validation tests or configure-time assertions.
9. Run all mandatory builds and QEMU gates in root `AGENTS.md`.
10. Update `BUILD.md` and target/profile documentation when user-visible configuration changes.

## Failure rule

When a required backend/target combination is not supported, return/configure an explicit unsupported result and fail closed. Do not silently enable a weaker backend or hardcode a platform assumption.
