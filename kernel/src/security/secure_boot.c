#include "secure_boot.h"

static const bharat_boot_policy_t g_boot_policy = {
#if defined(BHARAT_BOOT_HW_PROFILE_rtos)
    .security_level = BHARAT_BOOT_SECURITY_ENFORCED,
    .perf_mode = BHARAT_BOOT_PERF_ULTRA_FAST,
    .timer_tick_hz = 1000U,
    .smp_target_cores = 1U,
    .enable_zswap = 0U,
    .enable_ai_governor = 0U,
#elif defined(BHARAT_BOOT_HW_PROFILE_edge)
    .security_level = BHARAT_BOOT_SECURITY_ENFORCED,
    .perf_mode = BHARAT_BOOT_PERF_FAST,
    .timer_tick_hz = 500U,
    .smp_target_cores = 2U,
    .enable_zswap = 1U,
    .enable_ai_governor = 0U,
#elif defined(BHARAT_PROFILE_DRONE) || defined(BHARAT_PROFILE_ROBOT) || \
      defined(BHARAT_PROFILE_AUTOMOTIVE_ECU) || defined(BHARAT_PROFILE_AUTOMOTIVE_INFOTAINMENT)
    .security_level = BHARAT_BOOT_SECURITY_ENFORCED,
    .perf_mode = BHARAT_BOOT_PERF_ULTRA_FAST,
    .timer_tick_hz = 1000U,
    .smp_target_cores = 2U,
    .enable_zswap = 0U,
    .enable_ai_governor = 0U,
#else
    .security_level = BHARAT_BOOT_SECURITY_MEASURED,
    .perf_mode = BHARAT_BOOT_PERF_BALANCED,
    .timer_tick_hz = 100U,
    .smp_target_cores = 2U,
    .enable_zswap = 1U,
    .enable_ai_governor = 1U,
#endif
};

const bharat_boot_policy_t* bharat_boot_active_policy(void) {
    return &g_boot_policy;
}

int bharat_secure_boot_verify_early(void) {
    if (g_boot_policy.security_level == BHARAT_BOOT_SECURITY_DISABLED) {
        return 0;
    }

    return hal_secure_boot_arch_check(&g_boot_policy);
}
