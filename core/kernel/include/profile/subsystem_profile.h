#ifndef BHARAT_SUBSYSTEM_PROFILE_H
#define BHARAT_SUBSYSTEM_PROFILE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ISOLATION_MODE_NONE,
    ISOLATION_MODE_SANDBOX,
    ISOLATION_MODE_VIRTUALIZATION
} isolation_mode_t;

typedef enum {
    POWER_PROFILE_PERFORMANCE,
    POWER_PROFILE_BALANCED,
    POWER_PROFILE_SAVINGS
} power_profile_t;

typedef struct {
    const char* name;

    // Scheduling
    uint32_t default_policy;

    // Memory
    uint32_t mem_model;     // MMU / MPU / LITE
    bool has_iommu;

    // Isolation
    isolation_mode_t isolation;

    // Performance
    bool enable_preemption;
    bool enable_tracing;

    // Power
    power_profile_t power_mode;

} bharat_profile_t;


typedef enum {
    FAULT_POLICY_RESTART_SERVICE,
    FAULT_POLICY_ISOLATE,
    FAULT_POLICY_SAFE_MODE,
    FAULT_POLICY_REBOOT
} fault_policy_t;

typedef struct {
    int domain_id;
    fault_policy_t policy;
} fault_domain_t;


typedef struct subsystem_descriptor {
    const char* name;
    int (*init_fn)(void);
    uint32_t profile_mask;
    uintptr_t required_caps;
} subsystem_descriptor_t;

#define BHARAT_REGISTER_SUBSYSTEM(name, init, profiles, caps) \
    static const subsystem_descriptor_t __subsystem_##name \
    __attribute__((used, section("bharat_services"))) = { \
        .name = #name, \
        .init_fn = init, \
        .profile_mask = profiles, \
        .required_caps = caps \
    }

#endif // BHARAT_SUBSYSTEM_PROFILE_H

void init_subsystems(void);
