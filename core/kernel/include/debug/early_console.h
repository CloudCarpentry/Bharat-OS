#pragma once

#include <stddef.h>
#include <stdbool.h>
#include "drivers/serial/uart_driver.h"

void early_console_bind(uart_device_t *dev);
bool early_console_is_bound(void);
void early_console_putc(char c);
size_t early_console_write(const char *data, size_t len);
void early_console_flush(void);
