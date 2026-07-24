#ifndef BHARAT_BOOT_ARCH_BOOT_RAW_H
#define BHARAT_BOOT_ARCH_BOOT_RAW_H

#include <stdint.h>

// Raw architecture-specific boot arguments
typedef struct arch_boot_raw {
    // x86 Multiboot
    uint32_t mb_magic;
    uint32_t mb_info_ptr;

    // ARM / RISC-V FDT
    uint64_t hart_id; // For RISC-V, unused on ARM
    uint64_t fdt_ptr; // Physical address of the device tree blob

    // UEFI/EFI Handoff (if separate)
    uint64_t efi_system_table;
    uint64_t efi_image_handle;

    // Raw command line pointer (if passed out of band)
    uint64_t raw_cmdline_ptr;
} arch_boot_raw_t;

#endif // BHARAT_BOOT_ARCH_BOOT_RAW_H
