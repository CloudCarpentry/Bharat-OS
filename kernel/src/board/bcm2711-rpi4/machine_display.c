#include "bharat/display/display_caps.h"
#include <stdbool.h>

int machine_get_display_caps(machine_display_caps_t *out) {
    if (!out) return -1;

    out->display_present = true;
    out->boot_gui_allowed = true;
    out->firmware_fb_possible = true;
    out->early_simplefb_possible = true;
    out->early_panel_init_possible = false;
    out->needs_gpu_firmware = true;
    out->needs_complex_modeset = false;
    out->input_present = false;

    out->max_width = 1920;
    out->max_height = 1080;

    return 0;
}

int machine_probe_boot_video(display_probe_result_t *out, boot_video_handoff_t *video) {
    if (!out || !video) return -1;
    out->usable = false;
    return -1;
}
