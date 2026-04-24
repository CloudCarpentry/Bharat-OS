#include "bharat/drivers/generic_driver.h"

static bharat_generic_driver_status_t g_status = BHARAT_GENERIC_DRIVER_STATUS_UNINITIALIZED;

static int arch_irq_timer_init(void) {
    g_status = BHARAT_GENERIC_DRIVER_STATUS_READY;
    return 0;
}

static int arch_irq_timer_start(void) {
    if (g_status != BHARAT_GENERIC_DRIVER_STATUS_READY) {
        g_status = BHARAT_GENERIC_DRIVER_STATUS_FAILED;
        return -1;
    }

    return 0;
}

static bharat_generic_driver_status_t arch_irq_timer_health(void) {
    return g_status;
}

const bharat_generic_driver_ops_t bharat_arch_irq_timer_driver = {
    .name = "generic-arch-irq-timer",
    .class_id = BHARAT_GENERIC_DRIVER_CLASS_ARCH,
    .init = arch_irq_timer_init,
    .start = arch_irq_timer_start,
    .health = arch_irq_timer_health,
};
