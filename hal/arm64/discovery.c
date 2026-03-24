#include "hal/hal_discovery.h"
#include "hal/fdt_parser.h"
#include "boot/boot_info.h"

void hal_arch_discovery_init(const boot_info_t *boot) {
    if (!boot) return;

    system_discovery_t *discovery = hal_get_system_discovery();
    if (boot->firmware.fdt_ptr) {
        fdt_parse_discovery(boot->firmware.fdt_ptr, discovery);
    }

    if (discovery->topology.mem_region_count == 0) {
        discovery->topology.mem_regions[0].base = 0x40000000ULL;
        discovery->topology.mem_regions[0].size = 0x8000000ULL;
        discovery->topology.mem_regions[0].type = HAL_MEM_RAM;
        discovery->topology.mem_regions[0].node_id = 0;
        discovery->topology.mem_region_count = 1;
    }
}
