#include "hal/hal.h"
#include <stdint.h>

void hal_cpu_init(void) {}

uint64_t hal_cpu_get_fault_address(const void *trap_frame) {
    (void)trap_frame;
    // For ARM32, the Data Fault Address Register (DFAR) or Instruction Fault Address Register (IFAR)
    // could be read. We will return 0 as a stub for now.
    return 0;
}
