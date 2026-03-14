#include "board/shakti-ref/board.h"
#include "hal/hal.h"
#include "hal/hal_secure_boot.h"

/* Board-specific policy for SHAKTI Reference Board */
static const bharat_boot_policy_t g_shakti_policy = {
    .security_level = BHARAT_BOOT_SECURITY_MEASURED,
    .perf_mode = BHARAT_BOOT_PERF_FAST,
    .timer_tick_hz = 1000U,
    .smp_target_cores = 1U,
    .enable_zswap = 0U,
    .enable_ai_governor = 0U,
};

const bharat_boot_policy_t *hal_board_get_boot_policy(void) {
  return &g_shakti_policy;
}

void board_init(void) {
    hal_serial_init();
}
