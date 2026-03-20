#include <stdint.h>
#include <stddef.h>
#include "bharat_config.h"

void board_init(void)
{
    // Minimal stub for AVH Corstone-310 initialization.
    // In a full implementation, this would setup MPU/VTOR, UART (e.g., CMSDK UART),
    // and initialize system timers (e.g., SysTick).
}

// Basic serial stub for early printk
void board_early_putc(char c)
{
    // CMSDK UART0 usually at 0x49303000 on Corstone-300/310 non-secure
    // This is just a stub representation.
    volatile uint32_t *uart_tx = (volatile uint32_t *)0x49303000;
    *uart_tx = c;
}
