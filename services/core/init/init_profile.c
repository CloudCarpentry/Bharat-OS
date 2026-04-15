#include "init_profile.h"

// Bring in capability flags that might be needed for the mock population,
// eventually they should be moved to a generic uapi/init cap header, but for now we define locally if needed.
#ifndef BHARAT_INIT_CAP_NONE
#define BHARAT_INIT_CAP_NONE            (0)
#define BHARAT_INIT_CAP_NETWORK         (1 << 0)
#define BHARAT_INIT_CAP_STORAGE         (1 << 1)
#define BHARAT_INIT_CAP_DISPLAY         (1 << 2)
#define BHARAT_INIT_CAP_SENSORS         (1 << 3)
#define BHARAT_INIT_CAP_MMU             (1 << 4)
#endif

void init_profile_get_context(init_boot_context_t *ctx) {
    if (!ctx) return;

    // Default to TINY if nothing specified, to be safe
    ctx->boot_session_id = 0; // Mock default
    ctx->profile = INIT_PROFILE_TINY;
    ctx->arch_id = 0;
    ctx->platform_id = 0;
    ctx->board_id = 0;
    ctx->personality_id = 0;
    ctx->capability_mask = BHARAT_INIT_CAP_NONE;
    ctx->hw_feature_mask = 0;
    ctx->boot_reason = INIT_BOOT_REASON_UNKNOWN;
    ctx->reset_reason = 0;
    ctx->safe_mode_requested = false;
    ctx->diagnostics_requested = false;

#if defined(BHARAT_INIT_PROFILE_TINY)
    ctx->profile = INIT_PROFILE_TINY;
#elif defined(BHARAT_INIT_PROFILE_SMALL)
    ctx->profile = INIT_PROFILE_SMALL;
#elif defined(BHARAT_INIT_PROFILE_EMBEDDED_RICH)
    ctx->profile = INIT_PROFILE_EMBEDDED_RICH;
#elif defined(BHARAT_INIT_PROFILE_MOBILE)
    ctx->profile = INIT_PROFILE_MOBILE;
#elif defined(BHARAT_INIT_PROFILE_DESKTOP)
    ctx->profile = INIT_PROFILE_DESKTOP;
#elif defined(BHARAT_INIT_PROFILE_DRONE)
    ctx->profile = INIT_PROFILE_DRONE;
#elif defined(BHARAT_DEFAULT_INIT_PROFILE)
    // Fallback if built with BHARAT_DEFAULT_INIT_PROFILE string definition
    ctx->profile = INIT_PROFILE_DESKTOP;
#endif

    // Placeholder: populate capabilities based on environment
    // For now we assume all capabilities are present if not tiny
    if (ctx->profile != INIT_PROFILE_TINY) {
        ctx->capability_mask = BHARAT_INIT_CAP_NETWORK | BHARAT_INIT_CAP_STORAGE |
                        BHARAT_INIT_CAP_DISPLAY | BHARAT_INIT_CAP_SENSORS |
                        BHARAT_INIT_CAP_MMU;
    }
}
