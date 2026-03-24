# Boot Source Adapters

**Status**: Active
**Version**: 1.0

## 1. Abstraction Pattern
Rather than modifying the kernel flow (`kernel_boot.c`) per bootloader, Bharat-OS uses `boot_source_t` adapters. These map platform structures to the common `boot_info_t` before `kernel_main_common()` completes early validation.

## 2. Implementations
- **Multiboot2**: Maps GRUB/QEMU x86_64 boot structures.
- **Generic FDT**: Provides a shared parser for ARM64 and RISC-V device trees (`/memory`, `/chosen`, `linux,initrd-start`).
- **OpenSBI / U-Boot**: Wraps the generic FDT adapter while decorating `boot_info_t` with Hart IDs, architecture identifiers, or specific U-Boot security hints.
- **UEFI**: Included as a clean, compile-safe placeholder that explicitly returns `BOOT_ERR_UNSUPPORTED` pending full runtime services support.
