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
    const char* profile = kernel_boot_hw_profile();

    // 1. Initialize hardware architecture (CPU)
    hal_init();

    hal_serial_write("[bharat] kernel_main reached\n");
    hal_serial_write("[bharat] hw_profile=");
    hal_serial_write(profile);
    hal_serial_write("\n");
#if BHARAT_BOOT_GUI
    hal_serial_write("[bharat] boot_gui=ON\n");
#else
    hal_serial_write("[bharat] boot_gui=OFF\n");
#endif

    // 2. Initialize memory management (Paging, Physical Allocator)
    // 3. Initialize IPC mechanisms and threading

    // Halt the CPU loop
    while (1) {
        hal_cpu_halt();
    }
}
