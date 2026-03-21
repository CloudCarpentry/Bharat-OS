#include "bharat/drivers/generic_driver.h"

#ifndef BHARAT_ARRAY_SIZE
#define BHARAT_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

extern const bharat_generic_driver_ops_t bharat_arch_irq_timer_driver;
extern const bharat_generic_driver_ops_t bharat_board_fabric_driver;

static const bharat_generic_driver_ops_t *g_selected_drivers[] = {
#ifdef BHARAT_GENERIC_ENABLE_ARCH_IRQ_TIMER
    &bharat_arch_irq_timer_driver,
#endif
#ifdef BHARAT_GENERIC_ENABLE_BOARD_FABRIC
    &bharat_board_fabric_driver,
#endif
};

static bharat_generic_driver_summary_t g_summary;

int bharat_generic_driver_bootstrap(void) {
    g_summary.initialized = 0U;
    g_summary.started = 0U;
    g_summary.failed = 0U;

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_selected_drivers); ++i) {
        const bharat_generic_driver_ops_t *ops = g_selected_drivers[i];
        if (!ops || !ops->init) {
            g_summary.failed++;
            continue;
        }

        if (ops->init() == 0) {
            g_summary.initialized++;
            continue;
        }

        g_summary.failed++;
    }

    return (g_summary.failed == 0U) ? 0 : -1;
}

int bharat_generic_driver_start_all(void) {
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_selected_drivers); ++i) {
        const bharat_generic_driver_ops_t *ops = g_selected_drivers[i];
        if (!ops || !ops->start) {
            continue;
        }

        if (ops->start() == 0) {
            g_summary.started++;
            continue;
        }

        g_summary.failed++;
    }

    return (g_summary.failed == 0U) ? 0 : -1;
}

int bharat_generic_driver_summary(bharat_generic_driver_summary_t *out) {
    if (!out) {
        return -1;
    }

    *out = g_summary;
    return 0;
}
