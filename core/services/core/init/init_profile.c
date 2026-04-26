#include "init_profile.h"
#include <stddef.h>

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

static const init_profile_policy_t g_default_policy = {
    .name = "unknown",
    .strict_core_deadlines = false,
    .quiesce_after_handoff = false,
    .allow_optional_failure = true,
};

static const init_profile_policy_t g_profile_policies[] = {
    [0] = {0},
    [1] = { .name = "tiny", .strict_core_deadlines = false, .quiesce_after_handoff = false, .allow_optional_failure = true },
    [2] = { .name = "small", .strict_core_deadlines = false, .quiesce_after_handoff = false, .allow_optional_failure = true },
    [4] = { .name = "embedded_rich", .strict_core_deadlines = true, .quiesce_after_handoff = true, .allow_optional_failure = true },
    [8] = { .name = "rt", .strict_core_deadlines = true, .quiesce_after_handoff = false, .allow_optional_failure = true },
    [16] = { .name = "mobile", .strict_core_deadlines = true, .quiesce_after_handoff = true, .allow_optional_failure = true },
    [32] = { .name = "desktop", .strict_core_deadlines = false, .quiesce_after_handoff = false, .allow_optional_failure = true },
    [64] = { .name = "drone", .strict_core_deadlines = true, .quiesce_after_handoff = true, .allow_optional_failure = false },
    [128] = { .name = "cloud", .strict_core_deadlines = true, .quiesce_after_handoff = true, .allow_optional_failure = true },
    [256] = { .name = "automotive", .strict_core_deadlines = true, .quiesce_after_handoff = true, .allow_optional_failure = true },
    [512] = { .name = "tv", .strict_core_deadlines = false, .quiesce_after_handoff = true, .allow_optional_failure = true },
    [1024] = { .name = "appliance", .strict_core_deadlines = true, .quiesce_after_handoff = true, .allow_optional_failure = false },
    [2048] = { .name = "watch", .strict_core_deadlines = true, .quiesce_after_handoff = true, .allow_optional_failure = true },
};

const init_profile_policy_t *init_profile_get_policy(init_profile_t profile) {
    if (profile > 0 && profile < (init_profile_t)(sizeof(g_profile_policies) / sizeof(g_profile_policies[0]))) {
        const init_profile_policy_t *policy = &g_profile_policies[profile];
        if (policy->name != NULL) {
            return policy;
        }
    }
    return &g_default_policy;
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
