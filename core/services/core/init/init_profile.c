#include "init_profile.h"
#include <stddef.h>

#include <bharat/uapi/init/init_capability.h>

static const init_profile_policy_t g_default_policy = {
    .name = "unknown",
    .strict_core_deadlines = false,
    .quiesce_after_handoff = false,
    .allow_optional_failure = true,
};

static const init_profile_policy_t g_policy_tiny = { .name = "tiny", .strict_core_deadlines = false, .quiesce_after_handoff = false, .allow_optional_failure = true };
static const init_profile_policy_t g_policy_small = { .name = "small", .strict_core_deadlines = false, .quiesce_after_handoff = false, .allow_optional_failure = true };
static const init_profile_policy_t g_policy_embedded_rich = { .name = "embedded_rich", .strict_core_deadlines = true, .quiesce_after_handoff = true, .allow_optional_failure = true };
static const init_profile_policy_t g_policy_rt = { .name = "rt", .strict_core_deadlines = true, .quiesce_after_handoff = false, .allow_optional_failure = true };
static const init_profile_policy_t g_policy_mobile = { .name = "mobile", .strict_core_deadlines = true, .quiesce_after_handoff = true, .allow_optional_failure = true };
static const init_profile_policy_t g_policy_desktop = { .name = "desktop", .strict_core_deadlines = false, .quiesce_after_handoff = false, .allow_optional_failure = true };
static const init_profile_policy_t g_policy_drone = { .name = "drone", .strict_core_deadlines = true, .quiesce_after_handoff = true, .allow_optional_failure = false };
static const init_profile_policy_t g_policy_cloud = { .name = "cloud", .strict_core_deadlines = true, .quiesce_after_handoff = true, .allow_optional_failure = true };
static const init_profile_policy_t g_policy_automotive = { .name = "automotive", .strict_core_deadlines = true, .quiesce_after_handoff = true, .allow_optional_failure = true };
static const init_profile_policy_t g_policy_tv = { .name = "tv", .strict_core_deadlines = false, .quiesce_after_handoff = true, .allow_optional_failure = true };
static const init_profile_policy_t g_policy_appliance = { .name = "appliance", .strict_core_deadlines = true, .quiesce_after_handoff = true, .allow_optional_failure = false };
static const init_profile_policy_t g_policy_watch = { .name = "watch", .strict_core_deadlines = true, .quiesce_after_handoff = true, .allow_optional_failure = true };

const init_profile_policy_t *init_profile_get_policy(init_profile_t profile) {
    switch (profile) {
    case INIT_PROFILE_TINY:
        return &g_policy_tiny;
    case INIT_PROFILE_SMALL:
        return &g_policy_small;
    case INIT_PROFILE_EMBEDDED_RICH:
        return &g_policy_embedded_rich;
    case INIT_PROFILE_RT:
        return &g_policy_rt;
    case INIT_PROFILE_MOBILE:
        return &g_policy_mobile;
    case INIT_PROFILE_DESKTOP:
        return &g_policy_desktop;
    case INIT_PROFILE_DRONE:
        return &g_policy_drone;
    case INIT_PROFILE_CLOUD:
        return &g_policy_cloud;
    case INIT_PROFILE_AUTOMOTIVE:
        return &g_policy_automotive;
    case INIT_PROFILE_TV:
        return &g_policy_tv;
    case INIT_PROFILE_APPLIANCE:
        return &g_policy_appliance;
    case INIT_PROFILE_WATCH:
        return &g_policy_watch;
    default:
        return &g_default_policy;
    }
}

uint64_t init_profile_to_mask(init_profile_t profile) {
    return (uint64_t)profile;
}

bool init_profile_mask_allows(uint64_t mask, init_profile_t profile) {
    return (mask & (uint64_t)profile) != 0;
}

const char *init_profile_name(init_profile_t profile) {
    const init_profile_policy_t *policy = init_profile_get_policy(profile);
    return policy->name;
}

void init_profile_get_context(init_boot_context_t *ctx) {
    if (!ctx) return;

    // Initialize with safe defaults
    __builtin_memset(ctx, 0, sizeof(init_boot_context_t));

    ctx->abi_version = BHARAT_INIT_BOOT_CONTEXT_ABI_VERSION;
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
    ctx->failed_update_revert = false;

    // Kernel health summary default
    ctx->kernel_health.level = INIT_KERNEL_HEALTH_OK;
    ctx->kernel_health.failed_selftest_mask = 0;
    ctx->kernel_health.degraded_feature_mask = 0;

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
#elif defined(BHARAT_INIT_PROFILE_CLOUD)
    ctx->profile = INIT_PROFILE_CLOUD;
#elif defined(BHARAT_INIT_PROFILE_AUTOMOTIVE)
    ctx->profile = INIT_PROFILE_AUTOMOTIVE;
#elif defined(BHARAT_INIT_PROFILE_TV)
    ctx->profile = INIT_PROFILE_TV;
#elif defined(BHARAT_INIT_PROFILE_APPLIANCE)
    ctx->profile = INIT_PROFILE_APPLIANCE;
#elif defined(BHARAT_INIT_PROFILE_WATCH)
    ctx->profile = INIT_PROFILE_WATCH;
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
