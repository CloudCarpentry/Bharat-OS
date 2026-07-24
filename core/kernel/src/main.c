#include "hal/hal.h"
#include "console/console_core.h"
#include "boot/boot_info.h"
#include "boot/boot_validate.h"
#include "boot/boot_security.h"
#include <bharat/cpu_local.h>
#include "kernel_boot.h"
#include "boot/boot_mode.h"

extern void hal_serial_write(const char *s);

// The single unified kernel entry point called from architecture-specific entry code.
void kernel_main_common(const boot_info_t *boot) {
  // Setup core console layer and serial backend early
  console_early_init();
  console_write_raw("BOOT: kernel_main reached\n", 26);

  uint32_t cpu_id = hal_cpu_get_id();
  cpu_local_init(cpu_id);

  int is_bsp = (cpu_id == 0);

  if (is_bsp) {
      boot_common_early(boot);
      boot_common_security(boot);
      boot_common_memory(boot);
      boot_common_platform_services(boot);

      // EXPLICIT AUTOMOTIVE MODE FORCE FOR DEBUG
      ((struct boot_info*)boot)->selected_mode = BOOT_MODE_AUTOMOTIVE;

      boot_common_runtime(boot);
  } else {
      // AP boot path
      extern void smp_init(void);
      smp_init(); // Set up CPU-local data structures

      while (1) {
          hal_cpu_halt();
      }
  }
}
