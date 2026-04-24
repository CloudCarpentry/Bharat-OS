#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BHARAT_FAULT_RESTART_NONE = 0,
    BHARAT_FAULT_RESTART_THREAD = 1,
    BHARAT_FAULT_RESTART_DOMAIN = 2,
    BHARAT_FAULT_RESTART_SERVICE = 3,
    BHARAT_FAULT_REBOOT = 4,
} bharat_fault_restart_policy_t;

typedef enum {
    BHARAT_FAULT_ISOLATION_BEST_EFFORT = 0,
    BHARAT_FAULT_ISOLATION_STRONG = 1,
    BHARAT_FAULT_ISOLATION_HARD = 2,
} bharat_fault_isolation_level_t;

typedef enum {
    BHARAT_FAULT_SAFETY_STANDARD = 0,
    BHARAT_FAULT_SAFETY_IMPORTANT = 1,
    BHARAT_FAULT_SAFETY_CRITICAL = 2,
} bharat_fault_safety_level_t;

typedef struct {
    uint32_t version;
    uint32_t flags;

    uint32_t restart_policy;
    uint32_t isolation_level;
    uint32_t safety_level;
    uint32_t max_restart_count;

    uint64_t restart_window_ns;
    uint64_t supervisor_port;   // optional service endpoint/capability handle later

    uint64_t reserved[4];
} bharat_fault_domain_attr_t;

#ifdef __cplusplus
}
#endif
