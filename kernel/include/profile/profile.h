#ifndef BHARAT_PROFILE_H
#define BHARAT_PROFILE_H

/*
 * Bharat-OS profile selection and lightweight feature switches.
 *
 * This header intentionally keeps profile decisions build-time and small.
 * Runtime policy knobs are represented by bharat_boot_active_policy().
 */

void profile_init(void);

#if defined(CONFIG_PROFILE_OPENRAN)
#define FEATURE_HARD_REAL_TIME 1
#define FEATURE_ZERO_COPY_NET 1
#endif

#if defined(CONFIG_PROFILE_AUTOMOBILE) || defined(CONFIG_PROFILE_DRONE)
#define Profile_RTOS 1
#define FEATURE_SAFETY_CRITICAL 1
#endif

#if defined(CONFIG_PROFILE_MOBILE)
#define FEATURE_AGGRESSIVE_COW 1
#define FEATURE_AI_POWER_GOV 1
#define FEATURE_BINDER_IPC_HOOKS 1
#define FEATURE_SERVICE_MANAGER_HOOKS 1
#endif

#if defined(CONFIG_PROFILE_DATACENTER)
#define FEATURE_NUMA_AWARE 1
#define FEATURE_SCALABLE_SCHEDULER 1
#define FEATURE_OBSERVABILITY_PIPELINE 1
#endif

#if defined(CONFIG_PROFILE_NETWORK_APPLIANCE)
#define FEATURE_PACKET_FASTPATH 1
#define FEATURE_FLOW_TABLE_ACCEL 1
#define FEATURE_QOS_HOOKS 1
#endif

// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include <stdbool.h>
// Include the new canonical memory model contract
#include "mm/mem_model.h"

typedef enum {
    PROFILE_KERNEL_RT,
    PROFILE_KERNEL_GP,
    PROFILE_KERNEL_MIX
} KernelExecutionProfile;

/*
 * Returns true if the given profile is valid.
 */
bool kernel_execution_profile_is_valid(KernelExecutionProfile profile);

/*
 * Returns a human-readable name for the given profile ("RT", "GP", "MIX", or "UNKNOWN").
 */
const char *kernel_execution_profile_name(KernelExecutionProfile profile);

static inline KernelExecutionProfile get_kernel_execution_profile(void) {
#if defined(BHARAT_KERNEL_PROFILE_RT)
    return PROFILE_KERNEL_RT;
#elif defined(BHARAT_KERNEL_PROFILE_MIX)
    return PROFILE_KERNEL_MIX;
#else
    return PROFILE_KERNEL_GP;
#endif
}

/*
 * Returns the name of the active kernel execution profile.
 */
static inline const char *current_kernel_execution_profile_name(void) {
    return kernel_execution_profile_name(get_kernel_execution_profile());
}

// NOTE: This is a forward-looking policy structure.
// Not yet enforced in IPC/URPC routing.
typedef struct kernel_profile_comm_policy {
    KernelExecutionProfile profile;
    bool prefer_urpc_for_control;
    bool allow_ipc_for_bulk;
} kernel_profile_comm_policy_t;

typedef enum {
    PROFILE_TIER_A,
    PROFILE_TIER_B,
    PROFILE_TIER_C
} SystemProfile;

// Compatibility typedef and macros for the old MemoryModel enum
typedef mem_model_t MemoryModel;
#define MEM_MODEL_MMU MEM_MODEL_MMU_FULL
#define MEM_MODEL_FLAT MEM_MODEL_MPU

/**
 * Get the current memory model.
 * Wraps the new canonical API for backward compatibility.
 * Prefer `mem_model_get_current()` in new code.
 */
static inline MemoryModel get_memory_model(void) {
    return mem_model_get_current();
}

static inline SystemProfile get_system_profile(void) {
#if defined(Profile_RTOS)
    return PROFILE_TIER_A;
#elif defined(FEATURE_NUMA_AWARE)
    return PROFILE_TIER_C;
#else
    return PROFILE_TIER_B;
#endif
}

#endif /* BHARAT_PROFILE_H */
