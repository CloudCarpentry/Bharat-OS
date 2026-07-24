#ifndef BHARAT_BOOT_UI_TYPES_H
#define BHARAT_BOOT_UI_TYPES_H

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

#endif // BHARAT_BOOT_UI_TYPES_H
