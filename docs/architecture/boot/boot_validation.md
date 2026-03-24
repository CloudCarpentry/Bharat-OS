# Boot Validation Framework

**Status**: Active
**Version**: 1.0

## 1. Concept
Before `kernel_boot.c` executes, the canonical `boot_info_t` is aggressively validated by `boot_validate_all()`. This isolates parser errors, overlapping memory, and invalid cmdlines early.

## 2. Rules
1. **Memory Map**:
   - Size > 0.
   - Non-overlapping regions (checked via `BOOT_ERR_OVERLAPPING_MEM_RANGE`).
2. **Modules**:
   - Must not wraparound or possess size 0.
3. **Command Line**:
   - Capped at `BHARAT_BOOT_CMDLINE_MAX_LEN` and guaranteed to be null-terminated.
4. **Security Policy**:
   - Mismatched or invalid combinations (e.g. Verified without Secure Boot presence) reject the boot instantly.

## 3. Degraded vs Fatal
- Basic, Modules, Memory Map, and Security validation are **fatal** (`report.is_fatal = true`).
- Framebuffer geometry or missing RNG seeds trigger **degraded** boot (`bi->is_degraded = true`), allowing serial recovery or safe mode selection.
