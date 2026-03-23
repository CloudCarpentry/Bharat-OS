#include "hal/hal.h"
#include <stdint.h>

void hal_cpu_init(void) {}

bool hal_cpu_is_page_fault(const void *trap_frame) {
    (void)trap_frame;
    return false;
}

bool hal_cpu_is_fp_simd_fault(const void *trap_frame) {
    (void)trap_frame;
    return false;
}

bool hal_cpu_is_illegal_instruction(const void *trap_frame) {
    (void)trap_frame;
    return false;
}

uint32_t hal_interrupt_get_active_irq(uint64_t hw_cause) {
    (void)hw_cause;
    return hal_interrupt_acknowledge();
}

uint64_t hal_irq_timer_vector(void) {
    return 30U; // Generic ARM timer vector
}

uint64_t hal_cpu_get_fault_address(const void *trap_frame) {
    (void)trap_frame;
    // For ARM32, the Data Fault Address Register (DFAR) or Instruction Fault Address Register (IFAR)
    // could be read. We will return 0 as a stub for now.
    return 0;
}
