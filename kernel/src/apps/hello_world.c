#include "hal/hal.h"

void hello_world_app(void) {
  hal_serial_write("\n========================================\n");
  hal_serial_write("  Hello World from Bharat-OS Application!\n");
  hal_serial_write("========================================\n\n");
}
