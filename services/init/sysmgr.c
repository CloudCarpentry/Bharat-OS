#include "sysmgr.h"
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
    uint32_t current_profile = sysmgr_get_active_profile();

    bharat_runtime_log("sysmgr: Active profile bitmask resolved.");
    bharat_runtime_log("sysmgr: Discovering and filtering registered services...");

    const bharat_service_descriptor_t *svc = __start_bharat_services;
    while (svc < __stop_bharat_services) {
        if (svc->supported_profiles & current_profile) {
            // Service is allowed
            // Log starting message matching testing requirements
            bharat_runtime_log("sysmgr: started ");
            bharat_runtime_log(svc->name);
            if (svc->entry_point) {
                svc->entry_point();
            }
        } else {
            // Service is forbidden for this profile
            bharat_runtime_log("sysmgr: filtered ");
            bharat_runtime_log(svc->name);
            bharat_runtime_log(" (unsupported profile)");
        }
        svc++;
    }
}
