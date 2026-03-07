#include "kernel.h"
#include "hal/hal.h"

#include <stdint.h>

#ifndef PANIC_RECOVERY_MODE
// 0: Halt (Development), 1: Reboot (Production)
#define PANIC_RECOVERY_MODE 0
#endif

// Symbols defined in linker scripts for the PStore region
extern uint8_t _pstore_start;
extern uint8_t _pstore_end;

static void pstore_write(const char *msg) {
    // Only write if we have space. We do a very basic write without complex logging formatting.
    // Ensure we don't overflow the 4KB region. We can write a magic header or just the string.
    uint8_t *pstore = &_pstore_start;
    size_t max_len = (size_t)(&_pstore_end - &_pstore_start);
    size_t i = 0;

    // Zero out first to reset (in a real system we'd append or manage a ring buffer)
    for (size_t j = 0; j < 256 && j < max_len; j++) {
        pstore[j] = 0;
    }

    const char prefix[] = "KERNEL PANIC: ";
    while (prefix[i] != '\0' && i < max_len - 1) {
        pstore[i] = prefix[i];
        i++;
    }

    if (msg) {
        size_t j = 0;
        while (msg[j] != '\0' && i < max_len - 1) {
            pstore[i++] = msg[j++];
        }
    }
    pstore[i] = '\0';
}

void kernel_panic(const char *message) {
    hal_cpu_disable_interrupts();

    // UART dump
    hal_serial_write("\nKERNEL PANIC: ");
    hal_serial_write(message ? message : "(no message)");
    hal_serial_write("\n");

    // Dump architecture specific CPU state
    hal_cpu_dump_state();

    // Save to persistent storage
    pstore_write(message);

    if (PANIC_RECOVERY_MODE == 1) {
        hal_serial_write("Rebooting system...\n");
        hal_cpu_reboot();
    } else {
        hal_serial_write("System halted.\n");
        while (1) {
            hal_cpu_halt();
        }
    }
}
