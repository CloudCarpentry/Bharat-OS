#ifndef BHARAT_PROFILE_H
#define BHARAT_PROFILE_H

/*
 * Bharat-OS Profile Management
 * Defines feature toggles and compilation options based on the
 * target personality layer or deployment environment.
 *
 * Target profiles can be:
 * - PROFILE_OPENRAN: Hard real-time scheduling, zero-copy networking.
 * - PROFILE_AUTOMOBILE / PROFILE_DRONE: Safety critical, formal verification, RTOS defense.
 * - PROFILE_MOBILE: Aggressive CoW, AI power governors.
 * - PROFILE_DATACENTER: NUMA aware, high-throughput.
 */

// Example definition, this should be set by the build system (e.g., CMake)
// #define CONFIG_PROFILE_MOBILE 1

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
#endif

#if defined(CONFIG_PROFILE_DATACENTER)
    #define FEATURE_NUMA_AWARE 1
#endif

typedef enum {
    PROFILE_TIER_A,
    PROFILE_TIER_B,
    PROFILE_TIER_C
} SystemProfile;

// Mock function to get system profile
static inline SystemProfile get_system_profile(void) {
#if defined(Profile_RTOS)
    return PROFILE_TIER_A;
#elif defined(FEATURE_NUMA_AWARE)
    return PROFILE_TIER_C;
#else
    return PROFILE_TIER_B;
#endif
}

#endif // BHARAT_PROFILE_H
