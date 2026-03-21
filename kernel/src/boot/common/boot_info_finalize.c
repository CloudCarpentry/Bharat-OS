#include "bharat/boot_info.h"
#include <stdint.h>
#include <stdbool.h>

// Initialize global boot info structure based on raw input
void boot_info_finalize(boot_info_t *boot) {
    if (!boot) return;

    // Normalize memory map (e.g., merge adjacent regions, sort, etc)
    // Here we can sort the memory map if required by architectures

    // Calculate total memory
    uint64_t total_ram = 0;
    for (uint32_t i = 0; i < boot->mem_map_count; i++) {
        if (boot->mem_map[i].type == BOOT_MEM_USABLE) {
            total_ram += boot->mem_map[i].size;
        }
    }

    // Placeholder until boot_info_t stores aggregate RAM explicitly.
    (void)total_ram;
}
