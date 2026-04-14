#include "init_profile.h"
#include <bharat_config.h>

bharat_init_profile_t init_profile_get_active(void) {
    // Check defined profiles from build config
#if defined(BHARAT_INIT_PROFILE_TINY)
    return BHARAT_INIT_PROFILE_TINY;
#elif defined(BHARAT_INIT_PROFILE_SMALL)
    return BHARAT_INIT_PROFILE_SMALL;
#elif defined(BHARAT_INIT_PROFILE_EMBEDDED_RICH)
    return BHARAT_INIT_PROFILE_EMBEDDED_RICH;
#elif defined(BHARAT_INIT_PROFILE_MOBILE)
    return BHARAT_INIT_PROFILE_MOBILE;
#elif defined(BHARAT_INIT_PROFILE_DESKTOP)
    return BHARAT_INIT_PROFILE_DESKTOP;
#elif defined(BHARAT_INIT_PROFILE_DRONE)
    return BHARAT_INIT_PROFILE_DRONE;
#elif defined(BHARAT_DEFAULT_INIT_PROFILE)
    return BHARAT_DEFAULT_INIT_PROFILE;
#else
    // Default to DESKTOP if none is provided, per sysmgr fallback pattern
    return BHARAT_INIT_PROFILE_DESKTOP;
#endif
}
