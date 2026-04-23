# Status Code and Error Unification Contract

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: 🚧 Pending kernel/service behavior merge
- **Validated**: ❌ Pending stress/fault-injection tests


This document outlines the strict mapping rules between the internal kernel status space (`kstatus_t`) and the user-facing UAPI status space (`sys_errno_t`).

## 1. Spaces and Ownership

### 1.1 Kernel Internal Status (`kstatus_t`)
* **Type:** `int32_t`
* **Range:** `0` (Success) to negative values (`-1` and below).
* **Location:** `kernel/include/kernel/status.h`
* **Usage:** Used exclusively inside the kernel boundary, driver boundaries, and kernel-side subsystems.
* **Format:** Values are prefixed with `K_ERR_`.

### 1.2 User-Facing Status (`sys_errno_t`)
* **Type:** `int32_t`
* **Range:** Positive values (e.g., `1` for `SYS_EPERM`). The syscall boundary generally returns these as negative values (e.g., `-SYS_EPERM`).
* **Location:** `uapi/bharat/sys_errno.h`
* **Usage:** Used as the return code for syscalls, exposed to userspace applications and standard libraries.
* **Format:** Values are prefixed with `SYS_E`.

## 2. Translation and Boundaries

Kernel code **must never** return a raw `sys_errno_t` value directly from an internal function. Internal functions must return `kstatus_t`.

Translation from `kstatus_t` to `sys_errno_t` occurs **exclusively at the syscall boundary**. The following functions are responsible for this translation:
* `kstatus_to_sys_errno()`: Maps a `kstatus_t` to a positive `sys_errno_t`.
* `kstatus_to_sysret()`: Translates `kstatus_t` to a long integer representing a canonical syscall return value (`0` on success, `-sys_errno_t` on error).

## 3. Mapping Rules

Every `K_ERR_*` code should have a corresponding `SYS_E*` equivalent if it has a legitimate reason to be exposed to userspace.

* If a `K_ERR_*` does not cleanly map to a POSIX-like `SYS_E*`, it should map to `SYS_EINVAL` or `SYS_EIO` depending on context, or a new `SYS_E*` value should be added if absolutely necessary.
* **Do not duplicate mappings:** Ensure that multiple `K_ERR_*` codes don't accidentally map to a generic `SYS_EINVAL` if a more specific `SYS_E*` exists.
* **Names should be consistent:** If `K_ERR_INVALID_ARG` exists, it maps to `SYS_EINVAL`. If `K_ERR_NO_MEMORY` exists, it maps to `SYS_ENOMEM`.

## 4. Enforcement and Linting

A linting script (`tools/ci/check_status_codes.py`) runs in CI to verify the integrity of the status codes.

It ensures:
1. No duplicate numeric values exist within `status.h` or `sys_errno.h`.
2. No duplicate symbolic names exist.
3. Every `kstatus_t` explicitly defined in `status.h` has a considered mapping or is intentionally left internal.

*Any deviation or addition to `status.h` or `sys_errno.h` must pass this linting script.*
