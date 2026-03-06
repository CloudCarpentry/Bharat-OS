#include "kernel.h"
#include "hal/hal.h"

// Basic entry point for the microkernel
void kernel_main(void) {
    // 1. Initialize hardware architecture (CPU)
    hal_init();

    // 2. Initialize memory management (Paging, Physical Allocator)
    // 3. Initialize IPC mechanisms and threading
    
    // Halt the CPU loop
    while(1) {
        hal_cpu_halt();
    }
}
