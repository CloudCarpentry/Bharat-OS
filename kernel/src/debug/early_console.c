#include "debug/early_console.h"

static uart_device_t *g_early_console_dev = NULL;

void early_console_bind(uart_device_t *dev) {
    if (dev && dev->ops && dev->ops->init) {
        if (dev->ops->init(dev)) {
            g_early_console_dev = dev;
        }
    }
}

bool early_console_is_bound(void) {
    return g_early_console_dev != NULL;
}

void early_console_putc(char c) {
    if (g_early_console_dev && g_early_console_dev->ops && g_early_console_dev->ops->putc) {
        g_early_console_dev->ops->putc(g_early_console_dev, c);
    }
}

size_t early_console_write(const char *data, size_t len) {
    if (!g_early_console_dev || !g_early_console_dev->ops || !data) {
        return 0;
    }

    if (g_early_console_dev->ops->write) {
        return g_early_console_dev->ops->write(g_early_console_dev, data, len);
    } else if (g_early_console_dev->ops->putc) {
        for (size_t i = 0; i < len; i++) {
            g_early_console_dev->ops->putc(g_early_console_dev, data[i]);
        }
        return len;
    }
    return 0;
}

void early_console_flush(void) {
    if (g_early_console_dev && g_early_console_dev->ops && g_early_console_dev->ops->flush) {
        g_early_console_dev->ops->flush(g_early_console_dev);
    }
}
