#include "hal/hal.h"

__attribute__((unused)) static void hello_world_app(void) {
  hal_serial_write("\n========================================\n");
  hal_serial_write("  Hello World from Bharat-OS Application!\n");
  hal_serial_write("========================================\n\n");
}
