#include "board/bcm2711-rpi4/board.h"
#include "hal/hal.h"
#include "hal/hal_secure_boot.h"

/* Board-specific policy for BCM2711 Raspberry Pi 4 (ARM64) */
static const bharat_boot_policy_t g_bcm2711_policy = {
    .security_level = BHARAT_BOOT_SECURITY_MEASURED,
    .perf_mode = BHARAT_BOOT_PERF_FAST,
    .timer_tick_hz = 1000U,
    .smp_target_cores = 4U,
    .enable_zswap = 1U,
    .enable_ai_governor = 0U,
};

const bharat_boot_policy_t *hal_board_get_boot_policy(void) {
  return &g_bcm2711_policy;
}

void board_init(void) {
    hal_serial_init();
}
