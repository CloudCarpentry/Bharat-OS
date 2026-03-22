/*
 * machine_display.c — QEMU virt RISC-V display capabilities
 *
 * QEMU virt (RISC-V) has no native framebuffer at early boot.
 * VirtIO-GPU is available but requires a full driver (deferred to
 * userspace or a later boot stage).  We therefore report no display
 * and let the boot UI resolver fall gracefully back to text mode.
 *
 * The weak fallback in display_fallback.c already handles this, but
 * an explicit implementation here makes the board intent clear and
 * suppresses the "using weak symbol" behaviour which can mask bugs.
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
        out->display_present         = false;
        out->boot_gui_allowed        = false;
        out->firmware_fb_possible    = false;
        out->early_simplefb_possible = false;
        out->early_panel_init_possible = false;
        out->needs_gpu_firmware      = false;
        out->needs_complex_modeset   = false;
        out->input_present           = false;
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
        out->path            = BOOT_VIDEO_PATH_NONE;
        out->quality_score   = 0;
        out->early_usable    = false;
        out->interactive     = false;
        out->requires_takeover = false;

        if (video) {
            video->valid = false;
        }
    }

    return 0;
}
