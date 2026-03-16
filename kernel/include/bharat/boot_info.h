#ifndef BHARAT_BOOT_INFO_H
#define BHARAT_BOOT_INFO_H

#include <stdint.h>
#include <stdbool.h>
#include "bharat/display/boot_video.h"

typedef enum {
    BOOT_UI_NONE = 0,          // serial only
    BOOT_UI_TEXT,              // text console on display or serial
    BOOT_UI_SIMPLE_FB,         // framebuffer splash/progress
    BOOT_UI_EMBEDDED_UI,       // lightweight interactive early UI
    BOOT_UI_DEFERRED_GRAPHICS  // no early graphics; later compositor starts
} boot_ui_mode_t;

typedef enum {
    PROFILE_MINIMAL = 0,   // serial/text only
    PROFILE_EDGE,          // embedded UI allowed
    PROFILE_DESKTOP,       // compositor later
    PROFILE_DEV_VM,        // prefer simple graphics in VM
    PROFILE_KIOSK          // embedded full-screen UI
} system_profile_t;

// Boot Display Hooks inside Kernel
int boot_video_collect(boot_video_handoff_t *out);
int boot_video_validate(const boot_video_handoff_t *in);
int kernel_publish_boot_framebuffer(const boot_video_handoff_t *in, uint32_t *out_cap);
boot_ui_mode_t boot_ui_resolve_mode(system_profile_t profile);

#endif // BHARAT_BOOT_INFO_H
