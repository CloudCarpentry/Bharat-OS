/*
 * machine_display.c — QEMU virt ARM64 display capabilities
 *
 * QEMU virt (ARM64) can expose a VirtIO-GPU or a pl011 PL080 framebuffer.
 * With -device ramfb, we get a simple linear framebuffer that we can use
 * for the boot GUI without complex VirtIO negotiation.
 */
#include "bharat/display/display_caps.h"
#include "hal/hal_discovery.h"
#include <stdbool.h>

int machine_get_display_caps(machine_display_caps_t *out) {
    if (!out) return -1;

    system_discovery_t* discovery = hal_get_system_discovery();
    if (discovery && discovery->boot_video.valid) {
        out->display_present         = true;
        out->boot_gui_allowed        = true;
        out->firmware_fb_possible    = true;
        out->early_simplefb_possible = true;
        out->early_panel_init_possible = false;
        out->needs_gpu_firmware      = false;
        out->needs_complex_modeset   = false;
        out->input_present           = false;

        out->mmio_base   = discovery->boot_video.phys_addr;
        out->mmio_size   = discovery->boot_video.size;
        out->irq         = -1;
        out->max_width   = discovery->boot_video.width;
        out->max_height  = discovery->boot_video.height;
    } else {
        out->display_present         = true;
        out->boot_gui_allowed        = false; /* defer to userspace */
        out->firmware_fb_possible    = false;
        out->early_simplefb_possible = false;
        out->early_panel_init_possible = false;
        out->needs_gpu_firmware      = true;  /* VirtIO-GPU needs negotiation */
        out->needs_complex_modeset   = true;  /* modeset deferred */
        out->input_present           = false;

        out->mmio_base   = 0;
        out->mmio_size   = 0;
        out->irq         = -1;
        out->max_width   = 1920;
        out->max_height  = 1080;
    }

    return 0;
}

int machine_probe_boot_video(display_probe_result_t *out,
                             boot_video_handoff_t   *video) {
    if (!out) return -1;

    system_discovery_t* discovery = hal_get_system_discovery();
    if (discovery && discovery->boot_video.valid) {
        out->usable          = true;
        out->path            = BOOT_VIDEO_PATH_FIRMWARE_FB;
        out->quality_score   = 90;
        out->early_usable    = true;
        out->interactive     = false;
        out->requires_takeover = true;

        if (video) {
            *video = discovery->boot_video;
        }
    } else {
        out->usable          = false;
        out->path            = BOOT_VIDEO_PATH_VIRTIO_GPU_LATE;
        out->quality_score   = 0;
        out->early_usable    = false;
        out->interactive     = false;
        out->requires_takeover = true;

        if (video) {
            video->valid = false;
        }
    }

    return 0;
}
