#ifndef BHARAT_SECURE_BOOT_H
#define BHARAT_SECURE_BOOT_H

#include <stdint.h>

typedef enum {
    BHARAT_BOOT_SECURITY_DISABLED = 0,
    BHARAT_BOOT_SECURITY_MEASURED = 1,
    BHARAT_BOOT_SECURITY_ENFORCED = 2
} bharat_boot_security_level_t;

typedef enum {
    BHARAT_BOOT_PERF_BALANCED = 0,
    BHARAT_BOOT_PERF_FAST = 1,
    BHARAT_BOOT_PERF_ULTRA_FAST = 2
} bharat_boot_perf_mode_t;


typedef enum {
    BHARAT_BOOT_STAGE_ROM = 0,
    BHARAT_BOOT_STAGE_LOADER = 1,
    BHARAT_BOOT_STAGE_KERNEL = 2,
    BHARAT_BOOT_STAGE_INIT = 3
} bharat_boot_stage_t;

typedef struct {
    bharat_boot_security_level_t security_level;
    bharat_boot_perf_mode_t perf_mode;
    uint32_t timer_tick_hz;
    uint32_t smp_target_cores;
    uint8_t enable_zswap;
    uint8_t enable_ai_governor;
} bharat_boot_policy_t;

const bharat_boot_policy_t* bharat_boot_active_policy(void);
int bharat_secure_boot_verify_early(void);
int bharat_secure_boot_stage_hook(bharat_boot_stage_t stage, uint64_t measurement);
int hal_secure_boot_arch_check(const bharat_boot_policy_t* policy);

#endif
