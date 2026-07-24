#include "hal/hal_cpu.h"

int hal_cpu_init(uint32_t cpu_id) {
    /* RX CPU initialization */
    return 0;
}

void hal_cpu_halt(void) {
    /* RX wait instruction */
    __asm__ volatile("wait");
}
