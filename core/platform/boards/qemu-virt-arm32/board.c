#include "hal/hal.h"
#include "hal/hal_secure_boot.h"

/*
 * Board-specific boot policy for QEMU Virt (ARM 32-bit).
 *
 * This board runs EDGE/MMU_LITE profiles which are resource-constrained:
 *   - ZSWAP disabled: the slab allocator backing kcache_create is not
 *     available in the early MMU_LITE memory model.
 *   - AI governor disabled: no NEON/FPU governor calibration at this profile.
 *   - 2 SMP cores: matches QEMU virt ARM default core count.
 */
static const bharat_boot_policy_t g_qemu_arm32_policy = {
    .security_level    = BHARAT_BOOT_SECURITY_ENFORCED,
    .perf_mode         = BHARAT_BOOT_PERF_FAST,
    .timer_tick_hz     = 500U,
    .smp_target_cores  = 2U,
    .enable_zswap      = 0U,  /* MMU_LITE / EDGE: slab not available at boot */
    .enable_ai_governor = 0U,
};

const bharat_boot_policy_t *hal_board_get_boot_policy(void) {
    return &g_qemu_arm32_policy;
}

void board_init(void) {
    hal_serial_init();
}
