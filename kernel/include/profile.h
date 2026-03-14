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

typedef enum {
    MEM_MODEL_MMU,
    MEM_MODEL_MPU,
    MEM_MODEL_FLAT
} MemoryModel;

typedef enum {
    PROFILE_TIER_A,
    PROFILE_TIER_B,
    PROFILE_TIER_C
} SystemProfile;

static inline MemoryModel get_memory_model(void) {
#if defined(CONFIG_MEM_MODEL_MPU)
    return MEM_MODEL_MPU;
#elif defined(CONFIG_MEM_MODEL_FLAT)
    return MEM_MODEL_FLAT;
#else
    return MEM_MODEL_MMU;
#endif
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
