/*
 * machine_display.c — QEMU virt ARM64 display capabilities
 *
 * QEMU virt (ARM64) can expose a VirtIO-GPU or a pl011 PL080 framebuffer,
 * but neither is available at early boot without full driver initialisation.
 * We report display_present=true (the HW exists), but mark boot_gui_allowed
 * as false until a GPU firmware / complex modeset path is ready.
 *
 * This makes the boot UI resolver choose BOOT_UI_DEFERRED_GRAPHICS, causing
 * the kernel to skip early pixel rendering and pass the GPU resource to the
 * first userspace display server instead.
 */
#include "bharat/display/display_caps.h"
#include <stdbool.h>

int machine_get_display_caps(machine_display_caps_t *out) {
    if (!out) return -1;

    /*
     * GPU hardware exists (via VirtIO-GPU when running under QEMU), but
     * we cannot directly render pixels at early boot — the GPU requires
     * a VirtIO queue setup, MMIO negotiation, and resource allocation
     * that is too complex for pre-mm boot code.
     */
    out->display_present           = true;
    out->boot_gui_allowed          = false; /* defer to userspace */
    out->firmware_fb_possible      = false; /* no UEFI GOP on this path */
    out->early_simplefb_possible   = false; /* DT simplefb not configured */
    out->early_panel_init_possible = false;
    out->needs_gpu_firmware        = true;  /* VirtIO-GPU needs negotiation */
    out->needs_complex_modeset     = true;  /* modeset deferred */
    out->input_present             = false;

    out->mmio_base   = 0;  /* populated by DT/ACPI when driver runs */
    out->mmio_size   = 0;
    out->irq         = -1;
    out->max_width   = 1920;
    out->max_height  = 1080;

    return 0;
}

int machine_probe_boot_video(display_probe_result_t *out,
                             boot_video_handoff_t   *video) {
    if (!out) return -1;

    /* GPU needs complex init — boot video path is not available early. */
    out->usable          = false;
    out->path            = BOOT_VIDEO_PATH_VIRTIO_GPU_LATE; /* deferred */
    out->quality_score   = 0;  /* not yet usable */
    out->early_usable    = false;
    out->interactive     = false;
    out->requires_takeover = true; /* userspace must claim it */

    if (video) {
        video->valid = false;
    }

    return 0; /* probed successfully: deferred display path */
}
