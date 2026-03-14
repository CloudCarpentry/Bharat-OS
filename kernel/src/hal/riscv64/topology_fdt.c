#include "hal/hal_topology.h"
#include "hal/hal_boot.h"
#include "../common/fdt_parser.h"

int hal_topology_init(void) {
    bharat_boot_info_t* boot_info = hal_boot_get_info();
    if (boot_info && boot_info->fdt_base) {
        fdt_devices_t devices;
        if (fdt_parse(boot_info->fdt_base, boot_info, &devices) == 0) {
            // Success
            return 0;
        }
    }
    // Fallback to UMA
    return 0;
}
