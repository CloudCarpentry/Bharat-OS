#include "secure_boot.h"
#include "hal/hal_secure_boot.h"
#include "security/audit.h"

/* Default policy for generic hardware if no board-specific override exists */
static const bharat_boot_policy_t g_default_boot_policy
    __attribute__((aligned(8))) = {
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
#elif defined(BHARAT_BOOT_HW_PROFILE_mobile)
        .security_level = BHARAT_BOOT_SECURITY_ENFORCED,
        .perf_mode = BHARAT_BOOT_PERF_BALANCED,
        .timer_tick_hz = 250U,
        .smp_target_cores = 4U,
        .enable_zswap = 1U,
        .enable_ai_governor = 1U,
#elif defined(BHARAT_BOOT_HW_PROFILE_datacenter)
        .security_level = BHARAT_BOOT_SECURITY_MEASURED,
        .perf_mode = BHARAT_BOOT_PERF_FAST,
        .timer_tick_hz = 250U,
        .smp_target_cores = 8U,
        .enable_zswap = 1U,
        .enable_ai_governor = 1U,
#elif defined(BHARAT_BOOT_HW_PROFILE_network_appliance)
        .security_level = BHARAT_BOOT_SECURITY_ENFORCED,
        .perf_mode = BHARAT_BOOT_PERF_FAST,
        .timer_tick_hz = 1000U,
        .smp_target_cores = 4U,
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

/* Weak fallback implementation of board policy */
__attribute__((weak)) const bharat_boot_policy_t *
hal_board_get_boot_policy(void) {
  return &g_default_boot_policy;
}

const bharat_boot_policy_t *bharat_boot_active_policy(void) {
  return hal_board_get_boot_policy();
}

int bharat_secure_boot_verify_early(void) {
  const bharat_boot_policy_t *policy = bharat_boot_active_policy();
  if (!policy || policy->security_level == BHARAT_BOOT_SECURITY_DISABLED) {
    return 0;
  }

  return hal_secure_boot_arch_check(policy);
}

int bharat_secure_boot_stage_hook(bharat_boot_stage_t stage,
                                  uint64_t measurement) {
  return bharat_audit_record(BHARAT_AUDIT_EVENT_BOOT_MEASURE, 0U, 0,
                             (uint64_t)stage, measurement);
}
