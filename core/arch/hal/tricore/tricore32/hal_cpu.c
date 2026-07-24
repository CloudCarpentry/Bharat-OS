#include "hal/hal_cpu.h"

int hal_cpu_init(uint32_t cpu_id) {
    /* TriCore Context Management initialization */
    /* Set up CSA (Context Save Areas) */
    return 0;
}

void hal_cpu_halt(void) {
    /* TriCore wait instruction */
    __asm__ volatile("wait");
}
