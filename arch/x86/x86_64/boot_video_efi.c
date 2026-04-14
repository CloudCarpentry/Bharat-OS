#include "boot/boot_info.h"
#include "bharat/display/display_caps.h"
#include "bharat/display/boot_video.h"
#include "hal/hal.h"
#include "boot/multiboot2.h"
#include "kernel.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

static boot_video_handoff_t g_boot_video = {0};
static bool g_boot_video_parsed = false;

static uint32_t pci_read_config_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((((uint32_t)bus) << 16) | (((uint32_t)slot) << 11) |
                      (((uint32_t)func) << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
    __asm__ volatile("outl %0, %%dx" : : "a"(address), "d"((uint16_t)0xCF8));
    uint32_t tmp;
    __asm__ volatile("inl %%dx, %0" : "=a"(tmp) : "d"((uint16_t)0xCFC));
    return tmp;
}

void x86_64_scan_pci_for_vga(boot_video_handoff_t *out_handoff) {
    for (int bus = 0; bus < 8; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            uint32_t id = pci_read_config_32(bus, slot, 0, 0);
            if (id == 0x11111234 || id == 0xbeef80ee) { // QEMU/Bochs
                uint32_t bar0 = pci_read_config_32(bus, slot, 0, 0x10);
                if (bar0 & 0x1) continue; // Not memory BAR
                
                hal_serial_write("PCI: Found VGA framebuffer via BAR0 scan.\n");
                
                boot_video_handoff_t h = {0};
                h.valid = true;
                h.path = BOOT_VIDEO_PATH_FIRMWARE_FB;
                h.phys_addr = bar0 & 0xFFFFFFF0;
                h.virt_addr = h.phys_addr; // Default to identity before remapped
                h.width = 1024;
                h.height = 768;
                h.stride_bytes = 1024 * 4;
                h.size = h.height * h.stride_bytes;
                h.format = PIXEL_FORMAT_ARGB8888;
                h.source = BOOT_VIDEO_SOURCE_UNKNOWN;
                
                if (out_handoff) {
                    *out_handoff = h;
                } else {
                    g_boot_video = h;
                    g_boot_video_parsed = true;
                }
                return;
            }
        }
    }
}

// Call this from kernel_main / boot discovery to capture the framebuffer tag
void x86_64_parse_multiboot_framebuffer(multiboot_information_t *mb_info) {
    if (!mb_info) return;

    uint32_t total_size = mb_info->total_size;
    uint8_t *tag_ptr = (uint8_t *)mb_info + 8;
    while (tag_ptr < (uint8_t *)mb_info + total_size) {
        multiboot_tag_t *tag = (multiboot_tag_t *)((void *)tag_ptr);
        if (tag->type == MULTIBOOT_TAG_TYPE_END) {
            break;
        }

        if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
            multiboot_tag_framebuffer_t *fb_tag = (multiboot_tag_framebuffer_t *)((void *)tag);

            g_boot_video.valid = true;
            g_boot_video.path = BOOT_VIDEO_PATH_FIRMWARE_FB;
            g_boot_video.phys_addr = fb_tag->framebuffer_addr;
            g_boot_video.width = fb_tag->framebuffer_width;
            g_boot_video.height = fb_tag->framebuffer_height;
            g_boot_video.stride_bytes = fb_tag->framebuffer_pitch;
            g_boot_video.size = g_boot_video.height * g_boot_video.stride_bytes;

            if (fb_tag->framebuffer_bpp == 32) {
                // Approximate, we'd need to parse bitfield offsets for perfect mapping
                g_boot_video.format = PIXEL_FORMAT_ARGB8888;
            } else if (fb_tag->framebuffer_bpp == 16) {
                g_boot_video.format = PIXEL_FORMAT_RGB565;
            } else {
                g_boot_video.format = PIXEL_FORMAT_UNKNOWN;
            }

            g_boot_video.source = BOOT_VIDEO_SOURCE_UEFI_GOP;
            g_boot_video_parsed = true;
            break;
        }
        tag_ptr += ((tag->size + 7) & ~7);
    }
}

// Parse Multiboot 1 fallback
#include "boot/multiboot.h"

void x86_64_parse_multiboot1_framebuffer(void *mb_info) {
    if (!mb_info) return;
    multiboot1_info_t* mbi = (multiboot1_info_t*)mb_info;

    // Check if the framebuffer info is valid (bit 12 in flags)
    if (mbi->flags & (1 << 12)) {
        g_boot_video.valid = true;
        g_boot_video.path = BOOT_VIDEO_PATH_FIRMWARE_FB;
        g_boot_video.phys_addr = mbi->framebuffer_addr;
        g_boot_video.width = mbi->framebuffer_width;
        g_boot_video.height = mbi->framebuffer_height;
        g_boot_video.stride_bytes = mbi->framebuffer_pitch;
        g_boot_video.size = g_boot_video.height * g_boot_video.stride_bytes;

        if (mbi->framebuffer_bpp == 32) {
            g_boot_video.format = PIXEL_FORMAT_ARGB8888;
        } else if (mbi->framebuffer_bpp == 16) {
            g_boot_video.format = PIXEL_FORMAT_RGB565;
        } else {
            g_boot_video.format = PIXEL_FORMAT_UNKNOWN;
        }

        g_boot_video.source = BOOT_VIDEO_SOURCE_UEFI_GOP; // Using VBE but compatible mapping
        g_boot_video_parsed = true;
    }
}

// Implement machine contract using parsed video data
int machine_probe_boot_video(display_probe_result_t *out, boot_video_handoff_t *video) {
    if (!out || !video) return -1;

    if (g_boot_video_parsed && g_boot_video.valid) {
        *video = g_boot_video;
    } else {
        // Fallback: try PCI scan if no bootloader info found
        x86_64_scan_pci_for_vga(NULL);
        if (g_boot_video_parsed && g_boot_video.valid) {
            *video = g_boot_video;
        }
    }

    if (g_boot_video.valid) {

        out->usable = true;
        out->path = BOOT_VIDEO_PATH_FIRMWARE_FB;
        out->quality_score = 90;
        out->early_usable = true;
        out->interactive = false; // no early input yet
        out->requires_takeover = true;

        return 0;
    }

    out->usable = false;
    out->path = BOOT_VIDEO_PATH_NONE;
    out->quality_score = 0;

    return -1;
}

boot_video_handoff_t* boot_video_get_handoff_ptr(void) {
    return &g_boot_video;
}
