#include "sysmgr.h"
#include "init_runtime.h"
#include <bharat/runtime/runtime.h>

__attribute__((weak)) const bharat_service_descriptor_t __start_bharat_services[0];
__attribute__((weak)) const bharat_service_descriptor_t __stop_bharat_services[0];

// Simplified stub for now, ideally reads from kernel or build configuration
uint32_t sysmgr_get_active_profile(void) {
    uint32_t active = 0;
#if defined(BHARAT_PROFILE_DESKTOP)
    active |= PROFILE_DESKTOP;
#elif defined(BHARAT_PROFILE_EDGE)
    active |= PROFILE_EDGE;
#elif defined(BHARAT_PROFILE_RTOS)
    active |= PROFILE_RT;
#elif defined(BHARAT_PROFILE_MOBILE)
    active |= PROFILE_MOBILE;
#elif defined(BHARAT_PROFILE_AUTOMOTIVE)
    active |= PROFILE_AUTOMOTIVE;
#elif defined(BHARAT_PROFILE_APPLIANCE)
    active |= PROFILE_APPLIANCE;
#elif defined(BHARAT_PROFILE_TINY)
    active |= PROFILE_TINY_EMBEDDED;
#elif defined(BHARAT_PROFILE_CLOUD)
    active |= PROFILE_CLOUD;
#elif defined(BHARAT_PROFILE_SAFETY)
    active |= PROFILE_SAFETY;
#endif

    // Fallback if none defined
    if (active == 0) {
        active = PROFILE_DESKTOP; // Assume generic desktop default for tests
    }

    return active;
}

void sysmgr_enforce_startup_policy(void) {
    bharat_runtime_log("sysmgr: shim calling init_runtime_start...");
    init_boot_context_t ctx = {
        .profile = init_profile_get_active(),
        .arch_id = 0,
        .platform_id = 0,
        .board_id = 0,
        .personality_id = 0,
        .cap_mask = ~0ULL,
        .safe_mode = false,
        .diagnostics_mode = false,
    };

    init_runtime_start(&ctx);
}
