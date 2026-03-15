#include "hal/hal_discovery.h"

// Global discovery structure populated by early architecture boot code (ACPI or FDT)
static system_discovery_t g_system_discovery;

system_discovery_t* hal_get_system_discovery(void) {
    return &g_system_discovery;
}
