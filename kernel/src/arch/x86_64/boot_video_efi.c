#include "bharat/boot_info.h"
#include "bharat/display/display_caps.h"
#include "bharat/display/boot_video.h"
#include "../../boot/x86_64/multiboot2.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8

typedef struct {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint16_t reserved;
} multiboot_tag_framebuffer_t;

static boot_video_handoff_t g_boot_video = {0};
static bool g_boot_video_parsed = false;

// Call this from kernel_main / boot discovery to capture the framebuffer tag
__attribute__((unused)) static void x86_64_parse_multiboot_framebuffer(multiboot_information_t *mb_info) {
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

    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t color_info[6];
} multiboot1_info_t;

__attribute__((unused)) static void x86_64_parse_multiboot1_framebuffer(void *mb_info) {
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
