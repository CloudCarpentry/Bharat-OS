#ifndef BHARAT_UAPI_INIT_BOOT_CONTEXT_H
#define BHARAT_UAPI_INIT_BOOT_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    INIT_PROFILE_UNKNOWN = 0,
    INIT_PROFILE_TINY = 1,
    INIT_PROFILE_SMALL = 2,
    INIT_PROFILE_EMBEDDED_RICH = 4,
    INIT_PROFILE_RT = 8,
    INIT_PROFILE_MOBILE = 16,
    INIT_PROFILE_DESKTOP = 32,
    INIT_PROFILE_DRONE = 64,
    INIT_PROFILE_CLOUD = 128,
    INIT_PROFILE_AUTOMOTIVE = 256,
    INIT_PROFILE_TV = 512,
    INIT_PROFILE_APPLIANCE = 1024,
    INIT_PROFILE_WATCH = 2048,
} init_profile_t;

typedef enum {
    INIT_BOOT_REASON_UNKNOWN = 0,
    INIT_BOOT_REASON_COLD,
    INIT_BOOT_REASON_WARM,
    INIT_BOOT_REASON_WATCHDOG,
    INIT_BOOT_REASON_CRASH_RECOVERY,
    INIT_BOOT_REASON_UPDATE,
} init_boot_reason_t;

typedef struct {
    uint32_t boot_session_id;
    init_profile_t profile;
    uint32_t arch_id;
    uint32_t platform_id;
    uint32_t board_id;
    uint32_t personality_id;
    uint64_t capability_mask;
    uint64_t hw_feature_mask;
    init_boot_reason_t boot_reason;
    uint32_t reset_reason;
    bool safe_mode_requested;
    bool diagnostics_requested;
} init_boot_context_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_INIT_BOOT_CONTEXT_H
