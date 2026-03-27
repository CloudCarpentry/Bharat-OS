#include "boot/boot_info.h"
#include "bharat/display/boot_ui_types.h"
#include "bharat/display/display_caps.h"
#include "kernel.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

// Determines the optimal boot UI mode by considering profile constraints and board capability.
boot_ui_mode_t boot_ui_resolve_mode(system_profile_t profile) {
    boot_ui_mode_t requested_profile_mode = BOOT_UI_NONE;

    // Map system profile to requested mode
    switch(profile) {
        case PROFILE_MINIMAL:
            requested_profile_mode = BOOT_UI_NONE;
            break;
        case PROFILE_EDGE:
            requested_profile_mode = BOOT_UI_EMBEDDED_UI;
            break;
        case PROFILE_DESKTOP:
            requested_profile_mode = BOOT_UI_DEFERRED_GRAPHICS;
            break;
        case PROFILE_DEV_VM:
            requested_profile_mode = BOOT_UI_SIMPLE_FB;
            break;
        case PROFILE_KIOSK:
            requested_profile_mode = BOOT_UI_SIMPLE_FB;
            break;
        default:
            requested_profile_mode = BOOT_UI_TEXT;
            break;
    }

    // Probe machine capabilities
    machine_display_caps_t caps = {0};
    int ret = -1;
#if defined(__x86_64__) || defined(_M_X64)
    ret = machine_get_display_caps(&caps);
#endif
    if (ret != 0 || !caps.display_present || !caps.boot_gui_allowed) {
        return BOOT_UI_TEXT; // Or NATIVE
    }

    boot_ui_mode_t machine_max_supported_mode = BOOT_UI_NONE;
    if (caps.early_panel_init_possible) {
        machine_max_supported_mode = BOOT_UI_EMBEDDED_UI;
    } else if (caps.early_simplefb_possible || caps.firmware_fb_possible) {
        machine_max_supported_mode = BOOT_UI_SIMPLE_FB;
    } else if (caps.needs_complex_modeset || caps.needs_gpu_firmware) {
        machine_max_supported_mode = BOOT_UI_DEFERRED_GRAPHICS;
    } else {
        machine_max_supported_mode = BOOT_UI_TEXT;
    }

    // Calculate actual mode: min of requested vs machine capabilities
    boot_ui_mode_t resolved_mode = MIN(requested_profile_mode, machine_max_supported_mode);

#if !BHARAT_BOOT_GUI
    if (resolved_mode > BOOT_UI_TEXT) {
        resolved_mode = BOOT_UI_TEXT;
    }
#endif

    return resolved_mode;
}
