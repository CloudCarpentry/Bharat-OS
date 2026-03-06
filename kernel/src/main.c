#include "hal/hal.h"
#include "kernel.h"

/* Bring-up debug: serial output, visible with QEMU -nographic */
#ifdef __x86_64__
#define KPRINT(s) hal_serial_write(s)
#else
#define KPRINT(s) ((void)0)
#endif

static const char *kernel_boot_hw_profile(void) {
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
  const char *profile = kernel_boot_hw_profile();

  KPRINT("\n");
  KPRINT("  ██████╗ ██╗  ██╗ █████╗ ██████╗  █████╗ ████████╗\n");
  KPRINT("  ██╔══██╗██║  ██║██╔══██╗██╔══██╗██╔══██╗╚══██╔══╝\n");
  KPRINT("  ██████╔╝███████║███████║██████╔╝███████║   ██║\n");
  KPRINT("  ██╔══██╗██╔══██║██╔══██║██╔══██╗██╔══██║   ██║\n");
  KPRINT("  ██████╔╝██║  ██║██║  ██║██║  ██║██║  ██║   ██║\n");
  KPRINT("  ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝  ╚═╝\n");
  KPRINT("\n");
  KPRINT("  Bharat-OS  v0.1-dev  (x86_64 bring-up)\n");
  KPRINT("  Verification-first microkernel — made in India\n");
  KPRINT("\n");
  KPRINT("  [HAL] Initialising hardware...\n");

  // 1. Initialize hardware architecture (CPU)
  hal_init();

  KPRINT("  [HAL] Ready.\n");
  KPRINT("  [MK]  Entering halt loop (no scheduler yet).\n");
  KPRINT("\n");

  hal_serial_write("[bharat] kernel_main reached\n");
  hal_serial_write("[bharat] hw_profile=");
  hal_serial_write(profile);
  hal_serial_write("\n");
#if BHARAT_BOOT_GUI
  hal_serial_write("[bharat] boot_gui=ON\n");
#else
  hal_serial_write("[bharat] boot_gui=OFF\n");
#endif

  while (1) {
    hal_cpu_halt();
  }
}
