#ifndef BHARAT_MULTIBOOT_H
#define BHARAT_MULTIBOOT_H

#include <stdint.h>

/*
 * Bharat-OS Multiboot1 Specification Headers (x86_64)
 * Traditional Multiboot format used by legacy QEMU -kernel and GRUB.
 */

#define MULTIBOOT1_BOOTLOADER_MAGIC 0x2BADB002

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} multiboot1_info_t;

typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot1_mmap_entry_t;

#define MULTIBOOT1_FLAG_MMAP 0x00000040

#endif // BHARAT_MULTIBOOT_H
