#pragma once

#include "platform/device_profile.h"
#include "drivers/serial/uart_driver.h"

/* Helper to resolve a driver instance from a platform descriptor */
uart_device_t *serial_driver_match_boot_console(const platform_device_profile_t *profile);
