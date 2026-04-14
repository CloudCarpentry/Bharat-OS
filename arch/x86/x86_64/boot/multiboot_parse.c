#include "multiboot2.h"
#include "multiboot.h"
#include "boot/platform_boot_info.h"
#include "hal/hal_discovery.h"

extern void hal_serial_write(const char *s);
#define KPRINT(msg) hal_serial_write(msg)

// Forward declaration from arch code
extern void x86_64_parse_multiboot_framebuffer(multiboot_information_t *mb_info);

#include "bharat/display/boot_video.h"

// Forward declarations from arch code
extern void x86_64_parse_multiboot_framebuffer(multiboot_information_t *mb_info);
extern void x86_64_scan_pci_for_vga(boot_video_handoff_t *out_handoff);

static void parse_multiboot1(multiboot1_info_t *mb, boot_info_t *boot_info) {
    if (!(mb->flags & MULTIBOOT1_FLAG_MMAP)) return;

    multiboot1_mmap_entry_t *mmap = (multiboot1_mmap_entry_t *)(uint64_t)mb->mmap_addr;
    uint32_t mmap_end = mb->mmap_addr + mb->mmap_length;

    while ((uint32_t)(uint64_t)mmap < mmap_end) {
        if (boot_info->mem_region_count >= BHARAT_BOOT_MAX_MEM_REGIONS) break;

        boot_info->mem_regions[boot_info->mem_region_count].phys_start = mmap->addr;
        boot_info->mem_regions[boot_info->mem_region_count].size = mmap->len;

        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            boot_info->mem_regions[boot_info->mem_region_count].type = BOOT_MEM_USABLE;
        } else if (mmap->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) {
            boot_info->mem_regions[boot_info->mem_region_count].type = BOOT_MEM_ACPI_RECLAIM;
        } else {
            boot_info->mem_regions[boot_info->mem_region_count].type = BOOT_MEM_RESERVED;
        }

        boot_info->mem_region_count++;
        mmap = (multiboot1_mmap_entry_t *)((uint8_t *)mmap + mmap->size + sizeof(mmap->size));
    }

    if (mb->flags & 0x01) { // mem_lower/upper valid
        // Optional: use lower/upper if mmap is missing, but mmap is better.
    }

    if (mb->flags & MULTIBOOT1_FLAG_FRAMEBUFFER) {
        KPRINT("MB1: Framebuffer found\n");
        boot_info->console.type = BOOT_CONSOLE_FRAMEBUFFER;
        boot_info->console.fb_phys_base = mb->framebuffer_addr;
        boot_info->console.fb_width = mb->framebuffer_width;
        boot_info->console.fb_height = mb->framebuffer_height;
        boot_info->console.fb_pitch = mb->framebuffer_pitch;
        boot_info->console.fb_bpp = mb->framebuffer_bpp;
    } else {
        KPRINT("MB1: No Framebuffer flag, trying PCI scan...\n");
        boot_video_handoff_t h = {0};
        x86_64_scan_pci_for_vga(&h);
        if (h.valid) {
            boot_info->console.type = BOOT_CONSOLE_FRAMEBUFFER;
            boot_info->console.fb_phys_base = h.phys_addr;
            boot_info->console.fb_width = h.width;
            boot_info->console.fb_height = h.height;
            boot_info->console.fb_pitch = h.stride_bytes;
            boot_info->console.fb_bpp = 32; // Assuming 32 for now
            KPRINT("MB1: Found VGA via PCI\n");
        }
    }
}

static void parse_multiboot2(multiboot_information_t *mb_info, boot_info_t *boot_info) {
    KPRINT("MB2: Parsing tags...\n");
    // The first 8 bytes of mb_info are total_size and reserved
    multiboot_tag_t *tag = (multiboot_tag_t *)((uint8_t *)mb_info + 8);

    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        switch (tag->type) {
            case MULTIBOOT_TAG_TYPE_MMAP: {
                multiboot_tag_mmap_t *mmap = (multiboot_tag_mmap_t *)tag;
                uint32_t entry_count = (mmap->size - sizeof(multiboot_tag_mmap_t)) / mmap->entry_size;

                for (uint32_t i = 0; i < entry_count && boot_info->mem_region_count < BHARAT_BOOT_MAX_MEM_REGIONS; i++) {
                    multiboot_mmap_entry_t *entry = (multiboot_mmap_entry_t *)((uint8_t *)mmap->entries + i * mmap->entry_size);

                    boot_info->mem_regions[boot_info->mem_region_count].phys_start = entry->addr;
                    boot_info->mem_regions[boot_info->mem_region_count].size = entry->len;

                    if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
                        boot_info->mem_regions[boot_info->mem_region_count].type = BOOT_MEM_USABLE;
                    } else if (entry->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) {
                        boot_info->mem_regions[boot_info->mem_region_count].type = BOOT_MEM_ACPI_RECLAIM;
                    } else {
                        boot_info->mem_regions[boot_info->mem_region_count].type = BOOT_MEM_RESERVED;
                    }

                    boot_info->mem_region_count++;
                }
                break;
            }
            case MULTIBOOT_TAG_TYPE_CMDLINE: {
                multiboot_tag_string_t *cmd = (multiboot_tag_string_t *)tag;
                size_t len = tag->size - sizeof(multiboot_tag_t);
                if (len >= BHARAT_BOOT_CMDLINE_MAX_LEN) len = BHARAT_BOOT_CMDLINE_MAX_LEN - 1;
                for (size_t i = 0; i < len; i++) {
                    boot_info->cmdline[i] = cmd->string[i];
                }
                boot_info->cmdline[len] = '\0';
                break;
            }
            case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
                KPRINT("MB2: Framebuffer tag found\n");
                multiboot_tag_framebuffer_t *fb = (multiboot_tag_framebuffer_t *)tag;
                boot_info->console.type = BOOT_CONSOLE_FRAMEBUFFER;
                boot_info->console.fb_phys_base = fb->framebuffer_addr;
                boot_info->console.fb_width = fb->framebuffer_width;
                boot_info->console.fb_height = fb->framebuffer_height;
                boot_info->console.fb_pitch = fb->framebuffer_pitch;
                boot_info->console.fb_bpp = fb->framebuffer_bpp;
                break;
            }
        }
        tag = (multiboot_tag_t *)((uint8_t *)tag + ((tag->size + 7) & ~7));
    }

    if (boot_info->console.type != BOOT_CONSOLE_FRAMEBUFFER) {
        KPRINT("MB2: No Framebuffer tag, trying PCI scan...\n");
        boot_video_handoff_t h = {0};
        x86_64_scan_pci_for_vga(&h);
        if (h.valid) {
            boot_info->console.type = BOOT_CONSOLE_FRAMEBUFFER;
            boot_info->console.fb_phys_base = h.phys_addr;
            boot_info->console.fb_width = h.width;
            boot_info->console.fb_height = h.height;
            boot_info->console.fb_pitch = h.stride_bytes;
            boot_info->console.fb_bpp = 32;
            KPRINT("MB2: Found VGA via PCI\n");
        }
    }
}

void x86_64_parse_multiboot(uint32_t magic, void *mb_ptr, platform_boot_info_t *plat_info, boot_info_t *boot_info) {
    if (!mb_ptr || !boot_info || !plat_info) {
        hal_serial_write("MB: FATAL: Null pointers in parse_multiboot\n");
        return;
    }

    hal_serial_write("MB: magic: ");
    // Manual hex print for magic
    for (int i = 7; i >= 0; i--) {
        uint32_t nibble = (magic >> (i * 4)) & 0xF;
        char c = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        char buf[2] = {c, '\0'};
        hal_serial_write(buf);
    }
    hal_serial_write("\n");

    system_discovery_t *discovery = hal_get_system_discovery();
    discovery->topology.mem_region_count = 0;
    boot_info->mem_region_count = 0;
    boot_info->source = BOOT_SOURCE_MULTIBOOT2;
    boot_info->console.type = BOOT_CONSOLE_NONE;

    if (magic == MULTIBOOT2_BOOTLOADER_MAGIC) {
        KPRINT("MB: Multiboot 2 detected\n");
        parse_multiboot2((multiboot_information_t *)mb_ptr, boot_info);
    } else if (magic == MULTIBOOT1_BOOTLOADER_MAGIC) {
        KPRINT("MB: Multiboot 1 detected\n");
        parse_multiboot1((multiboot1_info_t *)mb_ptr, boot_info);
    } else {
        KPRINT("MB: Unknown magic number\n");
        return;
    }

    plat_info->has_early_console = true; // Fallback to safe serial defaults
}
