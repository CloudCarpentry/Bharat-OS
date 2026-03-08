#include "board/qemu-virt-arm64/board.h"
#include "hal/hal.h"
#include "hal/hal_secure_boot.h"

/* Board-specific policy for QEMU Virt (ARM64) */
static const bharat_boot_policy_t g_qemu_arm64_policy = {
    .security_level = BHARAT_BOOT_SECURITY_MEASURED,
    .perf_mode = BHARAT_BOOT_PERF_FAST,
    .timer_tick_hz = 1000U, /* Faster ticks for QEMU simulation */
    .smp_target_cores = 4U,
    .enable_zswap = 1U,
    .enable_ai_governor = 1U,
};

const bharat_boot_policy_t *hal_board_get_boot_policy(void) {
  return &g_qemu_arm64_policy;
}

void board_init(void) { hal_serial_init(); }
