#include "device.h"

#include <stddef.h>

static int driver_probe_noop(void) {
    return 0;
}

static void driver_irq_noop(uint32_t irq, void* ctx) {
    (void)irq;
    (void)ctx;
}

int device_register_builtin_drivers(void) {
    static const device_driver_t drivers[] = {
        {.name = "uart0", .class_id = DEVICE_CLASS_UART, .device_id = 0U, .probe = driver_probe_noop, .irq_handler = driver_irq_noop, .ctx = NULL},
        {.name = "spi0", .class_id = DEVICE_CLASS_SPI, .device_id = 0U, .probe = driver_probe_noop, .irq_handler = driver_irq_noop, .ctx = NULL},
        {.name = "i2c0", .class_id = DEVICE_CLASS_I2C, .device_id = 0U, .probe = driver_probe_noop, .irq_handler = driver_irq_noop, .ctx = NULL},
        {.name = "sdmmc0", .class_id = DEVICE_CLASS_SDMMC, .device_id = 0U, .probe = driver_probe_noop, .irq_handler = driver_irq_noop, .ctx = NULL},
        {.name = "eth0", .class_id = DEVICE_CLASS_ETHERNET, .device_id = 0U, .probe = driver_probe_noop, .irq_handler = driver_irq_noop, .ctx = NULL},
    };

    static const device_mmio_window_t windows[] = {
        { .class_id = DEVICE_CLASS_ETHERNET, .device_id = 0U, .window_id = 0U, .phys_base = 0x40000000U, .virt_base = 0x8000000000U, .size_bytes = 0x10000U, .irq = 10U, .in_use = 1U },
        { .class_id = DEVICE_CLASS_ETHERNET, .device_id = 0U, .window_id = 1U, .phys_base = 0x40010000U, .virt_base = 0x8000010000U, .size_bytes = 0x10000U, .irq = 10U, .in_use = 1U },
        { .class_id = DEVICE_CLASS_ETHERNET, .device_id = 1U, .window_id = 0U, .phys_base = 0x40020000U, .virt_base = 0x8000020000U, .size_bytes = 0x10000U, .irq = 11U, .in_use = 1U },
        { .class_id = DEVICE_CLASS_ETHERNET, .device_id = 1U, .window_id = 1U, .phys_base = 0x40030000U, .virt_base = 0x8000030000U, .size_bytes = 0x10000U, .irq = 11U, .in_use = 1U },
    };

    for (size_t i = 0; i < sizeof(drivers)/sizeof(drivers[0]); ++i) {
        if (device_register_driver(&drivers[i]) != 0) {
            return -1;
        }
    }

    for (size_t i = 0; i < sizeof(windows)/sizeof(windows[0]); ++i) {
        if (device_register_mmio_window(&windows[i]) != 0) {
            return -2;
        }
    }

    return 0;
}
