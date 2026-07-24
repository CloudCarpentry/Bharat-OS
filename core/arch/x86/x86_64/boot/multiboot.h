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

    /* Framebuffer info */
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t framebuffer_red_field_position;
    uint8_t framebuffer_red_mask_size;
    uint8_t framebuffer_green_field_position;
    uint8_t framebuffer_green_mask_size;
    uint8_t framebuffer_blue_field_position;
    uint8_t framebuffer_blue_mask_size;
} multiboot1_info_t;

#define MULTIBOOT1_FLAG_FRAMEBUFFER 0x00001000

typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot1_mmap_entry_t;

#define MULTIBOOT1_FLAG_MMAP 0x00000040

#endif // BHARAT_MULTIBOOT_H
