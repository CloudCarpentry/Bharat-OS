#ifndef BHARAT_SUBSYS_H
#define BHARAT_SUBSYS_H

#include <stdint.h>
#include "../../kernel/include/capability.h"
#include "hal/hal_cpu_topology.h"

/*
 * Bharat-OS Subsystem Abstraction Interface
 *
 * Supports launching foreign OS personalities either via API translation (Syscall interception)
 * or via lightweight Hardware Virtual Machines (Type-1 Hypervisor approach).
 */

typedef enum {
    SUBSYS_TYPE_LINUX = 1,
    SUBSYS_TYPE_WINDOWS = 2,
    SUBSYS_TYPE_BSD = 3,
    SUBSYS_TYPE_DARWIN = 4,
    SUBSYS_TYPE_POSIX_NATIVE = 5,
    SUBSYS_TYPE_ANDROID = 6
} subsys_type_t;

typedef enum {
    EXECUTION_MODE_API_TRANSLATION = 1,
    EXECUTION_MODE_VM = 2,
    EXECUTION_MODE_TESTING = 3
} subsys_exec_mode_t;

typedef enum {
    SUBSYS_ALLOC_DEFAULT = 0,
    SUBSYS_ALLOC_PACKED = 1,
    SUBSYS_ALLOC_SPREAD = 2
} subsys_alloc_policy_t;

typedef enum {
    SUBSYS_CLUSTER_PREF_NONE = 0,
    SUBSYS_CLUSTER_PREF_PERF = 1,
    SUBSYS_CLUSTER_PREF_EFF = 2
} subsys_cluster_preference_t;

typedef struct {
    subsys_alloc_policy_t policy;
    uint32_t preferred_cpu_mask;
    subsys_cluster_preference_t cluster_pref;
} subsys_alloc_config_t;

typedef struct {
    uint32_t subsys_id;
    subsys_type_t type;
    subsys_exec_mode_t exec_mode;

    // Limits and Configuration
    uint64_t memory_limit_mb;
    uint32_t requested_cpu_mask;
    uint32_t cpu_core_allocation_mask;
    subsys_alloc_config_t alloc_config;

    // The root capability representing this subsystem's isolated domain.
    uint32_t root_domain_cap;

    // Status
    int is_running;
} subsys_instance_t;

uint32_t subsys_resolve_effective_cpu_mask(
    const hal_cpu_topology_info_t* topology,
    uint32_t requested_mask,
    const subsys_alloc_config_t* config
);

// API to spawn a new subsystem instance
int subsys_create(subsys_type_t type, subsys_exec_mode_t mode, subsys_instance_t* out_instance);
int subsys_create_with_config(subsys_type_t type, subsys_exec_mode_t mode, const subsys_alloc_config_t* cfg, subsys_instance_t* out_instance);
int subsys_set_alloc_config(subsys_instance_t* instance, const subsys_alloc_config_t* cfg);

// Load the root filesystem or executable environment for the subsystem
int subsys_load_env(subsys_instance_t* instance, const char* root_path);

// Start the subsystem
int subsys_start(subsys_instance_t* instance);

// Stop and destroy the subsystem
int subsys_destroy(subsys_instance_t* instance);

#endif // BHARAT_SUBSYS_H
