#pragma once

#include "console_base_types.h"
#include "console_caps.h"

typedef struct uart_device uart_device_t;

typedef struct {
    bool (*init)(uart_device_t *dev);
    bool (*tx_ready)(uart_device_t *dev);
    void (*putc)(uart_device_t *dev, char c);
    size_t (*write)(uart_device_t *dev, const char *data, size_t len);
    void (*flush)(uart_device_t *dev);
    console_caps_t (*query_caps)(uart_device_t *dev);
} uart_driver_ops_t;

struct uart_device {
    const uart_driver_ops_t *ops;
    uintptr_t base;
    uint32_t reg_shift;
    uint32_t reg_width;
    uint32_t input_clock_hz;
    uint32_t baud_rate;
    void *opaque;
};
