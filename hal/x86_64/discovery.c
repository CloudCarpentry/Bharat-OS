#include "hal/hal_discovery.h"
#include "hal/hal.h"
#include "bharat/boot_info.h"
#include "console/console_core.h"

#include "console/console_types.h"

void hal_arch_discovery_init(const boot_info_t *boot) {
    if (!boot) return;

    system_discovery_t *discovery = hal_get_system_discovery();

    // x86_64 discovery currently relies on normalized boot info memory map
    discovery->topology.mem_region_count = 0;
    for (uint32_t i = 0; i < boot->mem_map_count && i < BHARAT_MAX_MEM_REGIONS; i++) {
        discovery->topology.mem_regions[i].base = boot->mem_map[i].phys_start;
        discovery->topology.mem_regions[i].size = boot->mem_map[i].size;

        // Map boot memory types to discovery types
        discovery->topology.mem_regions[i].type = (boot->mem_map[i].type == BOOT_MEM_USABLE) ? HAL_MEM_RAM : HAL_MEM_RESERVED;
        discovery->topology.mem_regions[i].node_id = 0; // x86_64 flat map for now

        discovery->topology.mem_region_count++;
    }

    console_log(CONSOLE_LEVEL_INFO, "HAL: x86_64: Discovery initialized. %u regions discovered.", discovery->topology.mem_region_count);

    // Copy video handoff if valid
    if (boot->video.valid) {
        discovery->boot_video = boot->video;
    }

    if (discovery->topology.mem_region_count == 0) {
        discovery->topology.mem_regions[0].base = 0x1000000ULL;
        discovery->topology.mem_regions[0].size = 0x8000000ULL;
        discovery->topology.mem_regions[0].type = HAL_MEM_RAM;
        discovery->topology.mem_regions[0].node_id = 0;
        discovery->topology.mem_region_count = 1;
        console_log(CONSOLE_LEVEL_INFO, "HAL: x86_64: No memory map from boot, using fallback region.");
    }
}
