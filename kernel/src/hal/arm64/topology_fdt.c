#include "hal/hal_topology.h"
#include "hal/hal_boot.h"
#include "../common/fdt_parser.h"

// Global boot info specific to ARM64
static bharat_boot_info_t g_arm64_boot_info;

bharat_boot_info_t* hal_boot_get_info(void) {
    return &g_arm64_boot_info;
}

int hal_topology_init(void) {
    if (g_arm64_boot_info.fdt_base) {
        fdt_devices_t devices;
        if (fdt_parse(g_arm64_boot_info.fdt_base, &g_arm64_boot_info, &devices) == 0) {
            // Success
            return 0;
        }
    }
    // Fallback to UMA
    return 0;
}
