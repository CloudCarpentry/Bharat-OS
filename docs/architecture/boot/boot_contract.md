# Canonical Boot Contract

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: 🚧 Pending kernel/service behavior merge
- **Validated**: ❌ Pending stress/fault-injection tests


**Status**: Active
**Version**: 1.0
**Owner**: Bharat-OS Core Team

## 1. Purpose
The Canonical Boot Contract defines `boot_info_t`, a uniform, early-stage handoff object. This struct unifies hardware-specific and loader-specific configurations (like OpenSBI/FDT, GRUB/Multiboot2, UEFI) before the generic kernel initialization code (`kernel_boot.c`) begins executing.

## 2. Structure
- **Magic**: `0xB4A2A705` validates that the structure was properly initialized.
- **Source/Arch**: Captures how the kernel was launched (e.g. `BOOT_SOURCE_OPENSBI_FDT`).
- **Memory Map**: Normalized list of `boot_memory_region_t`.
- **Modules**: Initrd, firmware blobs.
- **Console Handoff**: `boot_console_info_t` (serial or framebuffer).
- **Firmware Hints**: Pointers to raw FDT or ACPI RSDP.
- **Security Info**: Hardware measurements and secure boot presence flags.
- **Degraded Tracking**: `is_degraded` tracks non-fatal initialization/handoff errors.

## 3. Guarantees
- The struct relies strictly on fixed-size arrays (`BHARAT_BOOT_MAX_MEM_REGIONS` = 128) to avoid allocations before PMM.
- Boot mode (`BHARAT_BOOT_MODE_NORMAL`, `BHARAT_BOOT_MODE_RECOVERY`, etc) is resolved via `boot_mode_resolve` directly from this structure, isolating policy from parsing logic.

## 4. Current Status & Roadmap
*   **Status**: Baseline implementation complete.
*   **Roadmap**: We are currently focused on secure and measured boot, expanding upon the security info captured by this structure to enable secure updates and fallback control points. For detailed boot architecture code status and future roadmap, please see `docs/architecture/boot/boot-status-and-roadmap.md`.
