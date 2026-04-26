#include "hal/hal_cpu.h"
#include <stdbool.h>

int hal_cpu_init(uint32_t cpu_id) {
    // ARC specific CPU initialization
    // Set up status32, vector table base, etc.
    return 0;
}

void hal_cpu_halt(void) {
    __asm__ volatile("halt");
}

void hal_cpu_relax(void) {
    // ARC doesn't have a direct 'pause', usually just nop or sleep
    __asm__ volatile("nop");
}
