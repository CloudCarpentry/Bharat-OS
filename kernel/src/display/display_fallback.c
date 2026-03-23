#include "bharat/display/display_caps.h"
#include <stdbool.h>
#include <stddef.h>

// Weak global fallback: used if the board doesn't explicitly furnish display caps.
__attribute__((weak)) int machine_get_display_caps(machine_display_caps_t *out) {
    if (!out) return -1;
    out->display_present = false;
    out->boot_gui_allowed = false;
    out->firmware_fb_possible = false;
    out->early_simplefb_possible = false;
    out->early_panel_init_possible = false;
    out->needs_gpu_firmware = false;
    out->needs_complex_modeset = false;
    out->input_present = false;
    out->max_width = 0;
    out->max_height = 0;
    return 0; // successfully probed that we HAVE NO display
}

// Weak global fallback: used if the board doesn't provide boot video handoff details.
__attribute__((weak)) int machine_probe_boot_video(display_probe_result_t *out, boot_video_handoff_t *video) {
    if (!out) return -1;
    out->usable = false;
    out->path = BOOT_VIDEO_PATH_NONE;
    out->quality_score = 0;
    out->early_usable = false;
    out->interactive = false;
    out->requires_takeover = false;

    if (video) {
        video->valid = false;
    }

    return 0; // successfully probed that we HAVE NO video handoff
}
