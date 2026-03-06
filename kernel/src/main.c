#include "hal/hal.h"
#include "kernel.h"

/* Bring-up debug: COM1 serial output, visible in QEMU -nographic */
#ifdef __x86_64__
#include "hal/serial.h"
#define KPRINT(s) serial_puts(s)
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
  hal_init();
  KPRINT("  [HAL] Ready.\n");
  KPRINT("  [MK]  Entering halt loop (no scheduler yet).\n");
  KPRINT("\n");

  while (1) {
    hal_cpu_halt();
  }
}
