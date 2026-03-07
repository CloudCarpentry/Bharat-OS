#include "kernel.h"
#include "hal/hal.h"

void kernel_panic(const char *message) {
    hal_cpu_disable_interrupts();
    hal_serial_write("\nKERNEL PANIC: ");
    hal_serial_write(message ? message : "(no message)");
    hal_serial_write("\nSystem halted.\n");

    while (1) {
        hal_cpu_halt();
    }
}
