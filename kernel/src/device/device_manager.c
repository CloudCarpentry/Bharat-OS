#include "device.h"
#include "kernel_safety.h"

#include <stddef.h>

#define MAX_DEV_DRIVERS 32U
#define MAX_MMIO_WINDOWS 64U

static device_driver_t g_drivers[MAX_DEV_DRIVERS];
static device_mmio_window_t g_windows[MAX_MMIO_WINDOWS];

int device_framework_init(void) {
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_drivers); ++i) {
        g_drivers[i].name = NULL;
        g_drivers[i].probe = NULL;
        g_drivers[i].irq_handler = NULL;
        g_drivers[i].ctx = NULL;
        g_drivers[i].device_id = 0U;
        g_drivers[i].class_id = DEVICE_CLASS_UART;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_windows); ++i) {
        g_windows[i].in_use = 0U;
    }

    return 0;
}

int device_register_driver(const device_driver_t* driver) {
    if (!driver || !driver->name) {
        return -1;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_drivers); ++i) {
        if (!g_drivers[i].name) {
            g_drivers[i] = *driver;
            if (g_drivers[i].probe) {
                return g_drivers[i].probe();
            }
            return 0;
        }
    }

    return -2;
}

int device_register_mmio_window(const device_mmio_window_t* window) {
    if (!window || window->size_bytes == 0U) {
        return -1;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_windows); ++i) {
        if (g_windows[i].in_use == 0U) {
            g_windows[i] = *window;
            g_windows[i].in_use = 1U;
            return 0;
        }
    }

    return -2;
}

int device_lookup_mmio_window(device_class_t class_id,
                              uint32_t device_id,
                              uint32_t window_id,
                              device_mmio_window_t* out_window) {
    if (!out_window) {
        return -1;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_windows); ++i) {
        if (g_windows[i].in_use != 0U &&
            g_windows[i].class_id == class_id &&
            g_windows[i].device_id == device_id &&
            g_windows[i].window_id == window_id) {
            *out_window = g_windows[i];
            return 0;
        }
    }

    return -2;
}

int device_dispatch_irq(uint32_t irq) {
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_drivers); ++i) {
        if (g_drivers[i].name && g_drivers[i].irq_handler) {
            g_drivers[i].irq_handler(irq, g_drivers[i].ctx);
        }
    }

    return 0;
}
