#include "init_profile.h"

void init_profile_get_context(init_boot_context_t *ctx) {
    if (!ctx) return;

    // Default to TINY if nothing specified, to be safe
    ctx->profile = BHARAT_INIT_PROFILE_TINY;
    ctx->arch_id = 0;
    ctx->platform_id = 0;
    ctx->board_id = 0;
    ctx->personality_id = 0;
    ctx->cap_mask = BHARAT_INIT_CAP_NONE;
    ctx->safe_mode = false;
    ctx->diagnostics_mode = false;

#ifdef BHARAT_INIT_PROFILE_TINY
    ctx->profile = BHARAT_INIT_PROFILE_TINY;
#elif defined(BHARAT_INIT_PROFILE_SMALL)
    ctx->profile = BHARAT_INIT_PROFILE_SMALL;
#elif defined(BHARAT_INIT_PROFILE_EMBEDDED_RICH)
    ctx->profile = BHARAT_INIT_PROFILE_EMBEDDED_RICH;
#elif defined(BHARAT_INIT_PROFILE_MOBILE)
    ctx->profile = BHARAT_INIT_PROFILE_MOBILE;
#elif defined(BHARAT_INIT_PROFILE_DESKTOP)
    ctx->profile = BHARAT_INIT_PROFILE_DESKTOP;
#elif defined(BHARAT_INIT_PROFILE_DRONE)
    ctx->profile = BHARAT_INIT_PROFILE_DRONE;
#elif defined(BHARAT_DEFAULT_INIT_PROFILE)
    // Fallback if built with BHARAT_DEFAULT_INIT_PROFILE string definition
    // For now, this is a placeholder if a macro is defined as a string, e.g. "DESKTOP"
    // In a real build system, this would be converted to an enum or defined directly.
    ctx->profile = BHARAT_INIT_PROFILE_DESKTOP;
#endif

    // Placeholder: populate capabilities based on environment
    // For now we assume all capabilities are present if not tiny
    if (ctx->profile != BHARAT_INIT_PROFILE_TINY) {
        ctx->cap_mask = BHARAT_INIT_CAP_NETWORK | BHARAT_INIT_CAP_STORAGE |
                        BHARAT_INIT_CAP_DISPLAY | BHARAT_INIT_CAP_SENSORS |
                        BHARAT_INIT_CAP_MMU;
    }
}
