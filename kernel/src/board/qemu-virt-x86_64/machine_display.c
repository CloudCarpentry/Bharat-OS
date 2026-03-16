#include "bharat/display/display_caps.h"
#include "board/qemu-virt-x86_64/board.h"
#include <stdbool.h>

int machine_get_display_caps(machine_display_caps_t *out) {
    if (!out) return -1;

    out->display_present = true;
    out->boot_gui_allowed = true;
    out->firmware_fb_possible = true;
    out->early_simplefb_possible = false;
    out->early_panel_init_possible = false;
    out->needs_gpu_firmware = false;
    out->needs_complex_modeset = false; // At early boot, GOP is available
    out->input_present = false;         // We don't have early input drivers yet

    out->max_width = 1920;
    out->max_height = 1080;

    return 0;
}
