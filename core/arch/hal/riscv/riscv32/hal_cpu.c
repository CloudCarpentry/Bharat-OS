#include "hal/hal_capabilities.h"
#include <stdbool.h>

void hal_cpu_halt(void) {
    __asm__ volatile("wfi");
}

void hal_cpu_relax(void) {
    __asm__ volatile("nop");
}
