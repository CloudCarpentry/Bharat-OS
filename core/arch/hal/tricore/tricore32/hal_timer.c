#include "hal/hal_capabilities.h"
#include "kernel/status.h"

int hal_timer_source_init(uint32_t tick_hz) {
    (void)tick_hz;
    return hal_unsupported_op();
}
