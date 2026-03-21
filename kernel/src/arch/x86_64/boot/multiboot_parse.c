#include "boot/x86_64/multiboot2.h"
#include "boot/x86_64/multiboot.h"
#include "boot/platform_boot_info.h"
#include "hal/hal_discovery.h"

// Forward declaration from arch code
extern void x86_64_parse_multiboot_framebuffer(multiboot_information_t *mb_info);

static void parse_multiboot1(multiboot1_info_t *mb, boot_info_t *boot_info) {
    if (!(mb->flags & MULTIBOOT1_FLAG_MMAP)) return;

    multiboot1_mmap_entry_t *mmap = (multiboot1_mmap_entry_t *)(uint64_t)mb->mmap_addr;
    uint32_t mmap_end = mb->mmap_addr + mb->mmap_length;

    while ((uint32_t)(uint64_t)mmap < mmap_end) {
        if (boot_info->mem_map_count >= BHARAT_BOOT_MAX_MEM_REGIONS) break;

        boot_info->mem_map[boot_info->mem_map_count].phys_start = mmap->addr;
        boot_info->mem_map[boot_info->mem_map_count].size = mmap->len;
        
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            boot_info->mem_map[boot_info->mem_map_count].type = BOOT_MEM_USABLE;
        } else if (mmap->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) {
            boot_info->mem_map[boot_info->mem_map_count].type = BOOT_MEM_ACPI_RECLAIM;
        } else {
            boot_info->mem_map[boot_info->mem_map_count].type = BOOT_MEM_RESERVED;
        }

        boot_info->mem_map_count++;
        mmap = (multiboot1_mmap_entry_t *)((uint8_t *)mmap + mmap->size + sizeof(mmap->size));
    }

    if (mb->flags & 0x01) { // mem_lower/upper valid
        // Optional: use lower/upper if mmap is missing, but mmap is better.
    }
}

static void parse_multiboot2(multiboot_information_t *mb_info, boot_info_t *boot_info) {
    // The first 8 bytes of mb_info are total_size and reserved
    multiboot_tag_t *tag = (multiboot_tag_t *)((uint8_t *)mb_info + 8);

    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        switch (tag->type) {
            case MULTIBOOT_TAG_TYPE_MMAP: {
                multiboot_tag_mmap_t *mmap = (multiboot_tag_mmap_t *)tag;
                uint32_t entry_count = (mmap->size - sizeof(multiboot_tag_mmap_t)) / mmap->entry_size;
                
                for (uint32_t i = 0; i < entry_count && boot_info->mem_map_count < BHARAT_BOOT_MAX_MEM_REGIONS; i++) {
                    multiboot_mmap_entry_t *entry = (multiboot_mmap_entry_t *)((uint8_t *)mmap->entries + i * mmap->entry_size);
                    
                    boot_info->mem_map[boot_info->mem_map_count].phys_start = entry->addr;
                    boot_info->mem_map[boot_info->mem_map_count].size = entry->len;
                    
                    if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
                        boot_info->mem_map[boot_info->mem_map_count].type = BOOT_MEM_USABLE;
                    } else if (entry->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) {
                        boot_info->mem_map[boot_info->mem_map_count].type = BOOT_MEM_ACPI_RECLAIM;
                    } else {
                        boot_info->mem_map[boot_info->mem_map_count].type = BOOT_MEM_RESERVED;
                    }

                    boot_info->mem_map_count++;
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
        }
        tag = (multiboot_tag_t *)((uint8_t *)tag + ((tag->size + 7) & ~7));
    }
}

void x86_64_parse_multiboot(uint32_t magic, void *mb_ptr, platform_boot_info_t *plat_info, boot_info_t *boot_info) {
    if (!mb_ptr || !boot_info || !plat_info) return;

    system_discovery_t *discovery = hal_get_system_discovery();
    discovery->topology.mem_region_count = 0;
    boot_info->mem_map_count = 0;
    boot_info->booted_via_multiboot = true;

    if (magic == MULTIBOOT2_BOOTLOADER_MAGIC) {
        parse_multiboot2((multiboot_information_t *)mb_ptr, boot_info);
    } else if (magic == MULTIBOOT1_BOOTLOADER_MAGIC) {
        parse_multiboot1((multiboot1_info_t *)mb_ptr, boot_info);
    } else {
        return;
    }

    // For now, conservatively invalidate the video to force serial/text console
    // because full robust validation of multiboot video tags is missing here.
    boot_info->video.valid = false;
    plat_info->has_early_console = true; // Fallback to safe serial defaults
}
