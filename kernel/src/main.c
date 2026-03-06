#include "kernel.h"
#include "hal/hal.h"

static const char* kernel_boot_hw_profile(void) {
#if defined(BHARAT_BOOT_HW_PROFILE_desktop)
    return "desktop";
#elif defined(BHARAT_BOOT_HW_PROFILE_server)
    return "server";
#elif defined(BHARAT_BOOT_HW_PROFILE_vm)
    return "vm";
#elif defined(BHARAT_BOOT_HW_PROFILE_laptop)
    return "laptop";
#else
    return "generic";
#endif
}

// Basic entry point for the microkernel
void kernel_main(void) {
    (void)kernel_boot_hw_profile();

    // 1. Initialize hardware architecture (CPU)
    hal_init();

#if BHARAT_BOOT_GUI
    // Boot GUI hand-off point for a future userspace compositor.
    // Intentionally lightweight in ring-0 to preserve fast boot and small TCB.
#endif

    // 2. Initialize memory management (Paging, Physical Allocator)
    // 3. Initialize IPC mechanisms and threading

    // Halt the CPU loop
    while (1) {
        hal_cpu_halt();
    }
}
