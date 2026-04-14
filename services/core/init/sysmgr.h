#ifndef SYSMGR_H
#define SYSMGR_H

#include <stdint.h>

// Service definition structure for sysmgr to enforce profiles
typedef struct {
    const char *name;
    uint32_t type;               // e.g., SERVICE_TYPE_TELEMETRY
    uint32_t init_priority;      // Lower is earlier
    uint32_t required_caps;      // e.g., CAP_NET | CAP_STORAGE
    uint32_t supported_profiles; // e.g., PROFILE_GP | PROFILE_RT
    uint32_t restart_policy;     // e.g., RESTART_ISOLATE
    void (*entry_point)(void);
} bharat_service_descriptor_t;

// Profiles constants from profile matrix
#define PROFILE_TINY_EMBEDDED     (1 << 0)
#define PROFILE_APPLIANCE         (1 << 1)
#define PROFILE_DESKTOP           (1 << 2)
#define PROFILE_EDGE              (1 << 3)
#define PROFILE_CLOUD             (1 << 4)
#define PROFILE_RT                (1 << 5)
#define PROFILE_SAFETY            (1 << 6)
#define PROFILE_AUTOMOTIVE        (1 << 7)
#define PROFILE_MOBILE            (1 << 8)

// Registration Macro definition
#define BHARAT_REGISTER_SERVICE(var_name, ...) \
    __attribute__((section("bharat_services"))) \
    const bharat_service_descriptor_t var_name = __VA_ARGS__;

// Helper to get active profile (Legacy interface)
uint32_t sysmgr_get_active_profile(void);

// Sysmgr enforcement function (Legacy interface shimmed to init_runtime)
void sysmgr_enforce_startup_policy(void);

#endif // SYSMGR_H
