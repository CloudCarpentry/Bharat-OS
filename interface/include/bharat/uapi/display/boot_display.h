#ifndef BHARAT_UAPI_BOOT_DISPLAY_H
#define BHARAT_UAPI_BOOT_DISPLAY_H

#include <stdint.h>

/**
 * Early boot display states.
 * Defines the progression of the UI from boot to handoff.
 */
typedef enum {
    BHARAT_BOOT_DISPLAY_STATE_OFF = 0,
    BHARAT_BOOT_DISPLAY_STATE_SPLASH_ALLOWED = 1,
    BHARAT_BOOT_DISPLAY_STATE_DIAGNOSTIC_TEXT = 2,
    BHARAT_BOOT_DISPLAY_STATE_SECURE_PROMPT = 3,
    BHARAT_BOOT_DISPLAY_STATE_HANDOFF_TO_COMPOSITOR = 4,
} bharat_boot_display_state_t;

#endif /* BHARAT_UAPI_BOOT_DISPLAY_H */
