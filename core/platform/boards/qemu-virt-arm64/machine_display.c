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
#include <stdint.h>

static inline const system_discovery_t *machine_discovery_ptr(void) {
    const system_discovery_t *raw = hal_get_system_discovery();
    if (!raw) {
        return NULL;
    }

#if defined(__aarch64__)
    /*
     * Defensive canonicalization for early-boot callers: if the pointer carries
     * a top-byte tag while TBI is not enabled yet, strip it before dereference.
     */
    uintptr_t untagged = ((uintptr_t)raw) & UINT64_C(0x00ffffffffffffff);
    return (const system_discovery_t *)untagged;
#else
    return raw;
#endif
}

static bool machine_discovery_boot_video(const boot_video_handoff_t **video_out) {
    if (video_out) {
        *video_out = NULL;
    }

    const system_discovery_t *discovery = machine_discovery_ptr();
    if (!discovery) {
        return false;
    }

    const boot_video_handoff_t *video = &discovery->boot_video;
    if (!video->valid) {
        return false;
    }

    if (video->width == 0 || video->height == 0 || video->stride_bytes == 0) {
        return false;
    }

    uint64_t required = (uint64_t)video->height * (uint64_t)video->stride_bytes;
    if (video->phys_addr == 0 || video->size < required) {
        return false;
    }

    if (video_out) {
        *video_out = video;
    }

    return true;
}

int machine_get_display_caps(machine_display_caps_t *out) {
    if (!out) return -1;

    const boot_video_handoff_t *video = NULL;
    if (machine_discovery_boot_video(&video)) {
        out->display_present           = true;
        out->boot_gui_allowed          = true;
        out->firmware_fb_possible      = true;
        out->early_simplefb_possible   = true;
        out->early_panel_init_possible = false;
        out->needs_gpu_firmware        = false;
        out->needs_complex_modeset     = false;
        out->input_present             = false;

        out->mmio_base  = video->phys_addr;
        out->mmio_size  = video->size;
        out->irq        = -1;
        out->max_width  = video->width;
        out->max_height = video->height;
    } else {
        out->display_present           = true;
        out->boot_gui_allowed          = false; /* defer to userspace */
        out->firmware_fb_possible      = false;
        out->early_simplefb_possible   = false;
        out->early_panel_init_possible = false;
        out->needs_gpu_firmware        = true;  /* VirtIO-GPU needs negotiation */
        out->needs_complex_modeset     = true;  /* modeset deferred */
        out->input_present             = false;

        out->mmio_base  = 0;
        out->mmio_size  = 0;
        out->irq        = -1;
        out->max_width  = 1920;
        out->max_height = 1080;
    }

    return 0;
}

int machine_probe_boot_video(display_probe_result_t *out,
                             boot_video_handoff_t   *video) {
    if (!out) return -1;

    const boot_video_handoff_t *discovered = NULL;
    if (machine_discovery_boot_video(&discovered)) {
        out->usable            = true;
        out->path              = BOOT_VIDEO_PATH_FIRMWARE_FB;
        out->quality_score     = 90;
        out->early_usable      = true;
        out->interactive       = false;
        out->requires_takeover = true;

        if (video) {
            *video = *discovered;
        }
    } else {
        out->usable            = false;
        out->path              = BOOT_VIDEO_PATH_VIRTIO_GPU_LATE;
        out->quality_score     = 0;
        out->early_usable      = false;
        out->interactive       = false;
        out->requires_takeover = true;

        if (video) {
            video->valid = false;
        }
    }

    return 0;
}
