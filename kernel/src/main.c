#include "hal/hal.h"
#include "kernel.h"

#define KPRINT(s) hal_serial_write(s)

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
#if defined(__x86_64__) || defined(__i386__)
  KPRINT("  Bharat-OS  v0.1-dev  (x86_64 bring-up)\n");
#elif defined(__riscv)
  KPRINT("  Bharat-OS  v0.1-dev  (riscv64 bring-up)\n");
#elif defined(__aarch64__)
  KPRINT("  Bharat-OS  v0.1-dev  (arm64 bring-up)\n");
#else
  KPRINT("  Bharat-OS  v0.1-dev  (unknown arch bring-up)\n");
#endif
  KPRINT("  Verification-first microkernel — made in India\n");
  KPRINT("\n");
  KPRINT("  [HAL] Initialising hardware...\n");

  // 1. Initialize hardware architecture (CPU)
  hal_init();

  KPRINT("  [HAL] Ready.\n");

  KPRINT("  [MM]  Initializing memory...\n");
  // Proper memory map initialization will be passed from the bootloader.
  // We emit output to demonstrate the bring-up phase.
  KPRINT("  [MM]  Physical memory manager scaffolding initialized.\n");

  KPRINT("  [CPU] Enabling interrupts...\n");
  hal_cpu_enable_interrupts();
  KPRINT("  [CPU] Interrupts enabled.\n");

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
