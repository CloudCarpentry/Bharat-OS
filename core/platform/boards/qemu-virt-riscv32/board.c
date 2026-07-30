#include "hal/hal.h"
#include "hal/hal_secure_boot.h"

/*
 * Board-specific boot policy for QEMU Virt (RISC-V 32-bit).
 *
 * This board runs EDGE/MMU_LITE profiles which are resource-constrained:
 *   - ZSWAP disabled: the slab allocator backing kcache_create is not
 *     available in the early MMU_LITE memory model.
 *   - AI governor disabled: no vector FPU calibration available on RV32.
 *   - 2 SMP cores: matches QEMU virt default hart count for RV32.
 */
static const bharat_boot_policy_t g_qemu_riscv32_policy = {
    .security_level    = BHARAT_BOOT_SECURITY_ENFORCED,
    .perf_mode         = BHARAT_BOOT_PERF_FAST,
    .timer_tick_hz     = 500U,
    .smp_target_cores  = 2U,
    .enable_zswap      = 0U,  /* MMU_LITE / EDGE: slab not available at boot */
    .enable_ai_governor = 0U,
};

const bharat_boot_policy_t *hal_board_get_boot_policy(void) {
    return &g_qemu_riscv32_policy;
}

void board_init(void) {
    hal_serial_init();
}
