# Bharat-OS Naming Conventions

This document outlines the naming conventions for Bharat-OS to ensure consistency across the codebase, especially for UAPI and new subsystems.

## Layer-based Prefix Rules

| Layer                               | Prefix                                        |
| ----------------------------------- | --------------------------------------------- |
| `interface/include/bharat/uapi/...` | `bharat_`                                     |
| `core/lib/gfx/...` internal helpers | `bharat_` or `gfx_`, but prefer `bharat_gfx_` |
| Private static functions            | Local style is fine (e.g., `_camelCase` or `snake_case`) |
| Build flags                         | `BHARAT_BUILD_*`                              |
| Macros/Constants                    | `BHARAT_*`                                    |

## Detailed Rules

### 1. Public UAPI (Userspace API)
All types, enums, and functions exposed in `interface/include/bharat/uapi/` must use the `bharat_` prefix.

**Example:**
```c
typedef enum {
    BHARAT_DISPLAY_CLASS_FRAMEBUFFER = 1,
    BHARAT_DISPLAY_CLASS_GPU = 2,
    BHARAT_DISPLAY_CLASS_VIRTIO_GPU = 3,
    BHARAT_DISPLAY_CLASS_PANEL = 4,
    BHARAT_DISPLAY_CLASS_HEADLESS = 5,
} bharat_display_class_t;
```

### 2. Constants and Macros
Use uppercase with underscores and the `BHARAT_` prefix.

**Example:**
```c
#define BHARAT_BOOT_DISPLAY_STATE_OFF 0
```

### 3. Build Flags
Use uppercase with underscores and the `BHARAT_BUILD_` prefix.

**Example:**
```cmake
option(BHARAT_BUILD_INPUTD "Build Bharat input daemon" OFF)
```

### 4. Graphics Library (`core/lib/gfx`)
For internal graphics helpers, prefer `bharat_gfx_` prefix to avoid collisions.

**Example:**
```c
bool bharat_gfx_framebuffer_is_valid(const bharat_framebuffer_t *fb);
```

## Rationale
Consistency in naming is critical for ABI stability and documentation. While shorter prefixes like `bh_` might be tempting, `bharat_` is the established convention for public boundaries in this repository.
