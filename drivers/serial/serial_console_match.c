#include "drivers/serial/serial_provider.h"
#include "drivers/serial/uart_driver.h"
#include <stddef.h>

#if BHARAT_ENABLE_DRIVER_SERIAL_NS16550
#include "drivers/serial/ns16550.h"
#endif

#if BHARAT_ENABLE_DRIVER_SERIAL_PL011
#include "drivers/serial/pl011.h"
#endif

#if BHARAT_ENABLE_DRIVER_SERIAL_SIFIVE
#include "drivers/serial/sifive_uart.h"
#endif

#if BHARAT_ENABLE_DRIVER_SERIAL_CADENCE
#include "drivers/serial/cadence_uart.h"
#endif

#if BHARAT_ENABLE_DRIVER_SERIAL_IMX_LPUART
#include "drivers/serial/imx_lpuart.h"
#endif

#if BHARAT_ENABLE_DRIVER_SERIAL_DW_APB
#include "drivers/serial/dw_apb_uart.h"
#endif


static uart_device_t g_boot_uart_device;

static bool str_eq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return (*a == *b);
}

uart_device_t *serial_driver_match_boot_console(const platform_device_profile_t *profile) {
    if (!profile || !profile->has_uart) {
        return NULL;
    }

    const platform_uart_desc_t *desc = &profile->boot_console;

    g_boot_uart_device.base = desc->base_address;
    g_boot_uart_device.reg_shift = desc->reg_shift;
    g_boot_uart_device.reg_width = desc->reg_width;
    g_boot_uart_device.input_clock_hz = desc->input_clock_hz;
    g_boot_uart_device.baud_rate = desc->baud_rate;
    g_boot_uart_device.opaque = NULL;

    const char *name = desc->driver_name;
    if (!name) return NULL;

#if BHARAT_ENABLE_DRIVER_SERIAL_NS16550
    if (str_eq(name, "ns16550")) {
        g_boot_uart_device.ops = &uart_ns16550_ops;
        return &g_boot_uart_device;
    }
#endif

#if BHARAT_ENABLE_DRIVER_SERIAL_PL011
    if (str_eq(name, "pl011")) {
        g_boot_uart_device.ops = &uart_pl011_ops;
        return &g_boot_uart_device;
    }
#endif

#if BHARAT_ENABLE_DRIVER_SERIAL_SIFIVE
    if (str_eq(name, "sifive_uart")) {
        g_boot_uart_device.ops = &uart_sifive_ops;
        return &g_boot_uart_device;
    }
#endif

#if BHARAT_ENABLE_DRIVER_SERIAL_CADENCE
    if (str_eq(name, "cadence_uart")) {
        g_boot_uart_device.ops = &uart_cadence_ops;
        return &g_boot_uart_device;
    }
#endif

#if BHARAT_ENABLE_DRIVER_SERIAL_IMX_LPUART
    if (str_eq(name, "imx_lpuart")) {
        g_boot_uart_device.ops = &uart_imx_lpuart_ops;
        return &g_boot_uart_device;
    }
#endif

#if BHARAT_ENABLE_DRIVER_SERIAL_DW_APB
    if (str_eq(name, "dw_apb_uart")) {
        g_boot_uart_device.ops = &uart_dw_apb_ops;
        return &g_boot_uart_device;
    }
#endif

    return NULL;
}
