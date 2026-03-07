#ifndef BHARAT_MULTIBOOT2_H
#define BHARAT_MULTIBOOT2_H

#include <stdint.h>

/*
 * Bharat-OS Multiboot2 Specification Headers (x86_64)
 * Used by GRUB2 or Limine to load the kernel into memory, providing
 * memory maps, ACPI tables, and framebuffer information from UEFI/BIOS.
 */

#define MULTIBOOT2_HEADER_MAGIC 0xE85250D6
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

// The initial Header structure embedded at the beginning of kernel.elf
typedef struct {
    uint32_t magic;
    uint32_t architecture; // 0 for i386/x86_64
    uint32_t header_length;
    uint32_t checksum;
} multiboot_header_t;

// Tag structure provided by the Bootloader in register EBX upon entry
typedef struct {
    uint32_t total_size;
    uint32_t reserved;
} multiboot_information_t;

typedef struct {
    uint32_t type;
    uint32_t size;
} multiboot_tag_t;

#define MULTIBOOT_TAG_TYPE_END               0
#define MULTIBOOT_TAG_TYPE_MMAP              6

#define MULTIBOOT_MEMORY_AVAILABLE           1
#define MULTIBOOT_MEMORY_RESERVED            2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE    3
#define MULTIBOOT_MEMORY_NVS                 4
#define MULTIBOOT_MEMORY_BADRAM              5

typedef struct {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} multiboot_mmap_entry_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    multiboot_mmap_entry_t entries[0];
} multiboot_tag_mmap_t;

// Standard entry point called by the Bootloader (ASM -> C transition)
void kernel_main(uint32_t magic, multiboot_information_t* mb_info);

#endif // BHARAT_MULTIBOOT2_H
