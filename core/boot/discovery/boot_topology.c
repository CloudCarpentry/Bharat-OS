#include "hal/hal_topology.h"
#include "hal/hal_boot.h"

// Basic UMA fallback topology logic
int hal_topology_init(void) {
    bharat_boot_info_t* boot_info = hal_boot_get_info();
    if (!boot_info) return -1;

    // Check if firmware provided node information
    bool has_numa = false;
    for (uint32_t i = 0; i < boot_info->mem_region_count; ++i) {
        if (boot_info->mem_regions[i].numa_node != 0) {
            has_numa = true;
            break;
        }
    }

    if (!has_numa) {
        // Fallback to UMA (Node 0 for all CPUs and Memory)
        for (uint32_t i = 0; i < boot_info->cpu_count; ++i) {
            boot_info->cpus[i].numa_node = 0;
        }
        for (uint32_t i = 0; i < boot_info->mem_region_count; ++i) {
            boot_info->mem_regions[i].numa_node = 0;
        }
    }

    return 0;
}

uint32_t hal_topology_get_cpu_node(uint32_t cpu_id) {
    bharat_boot_info_t* boot_info = hal_boot_get_info();
    if (boot_info && cpu_id < boot_info->cpu_count) {
        return boot_info->cpus[cpu_id].numa_node;
    }
    return 0; // Default to node 0
}

uint32_t hal_topology_get_mem_node(uint64_t paddr) {
    bharat_boot_info_t* boot_info = hal_boot_get_info();
    if (boot_info) {
        for (uint32_t i = 0; i < boot_info->mem_region_count; ++i) {
            if (paddr >= boot_info->mem_regions[i].base &&
                paddr < boot_info->mem_regions[i].base + boot_info->mem_regions[i].size) {
                return boot_info->mem_regions[i].numa_node;
            }
        }
    }
    return 0; // Default to node 0
}

bool hal_topology_is_numa(void) {
    bharat_boot_info_t* boot_info = hal_boot_get_info();
    if (!boot_info) return false;

    // Check if firmware provided node information
    for (uint32_t i = 0; i < boot_info->mem_region_count; ++i) {
        if (boot_info->mem_regions[i].numa_node != 0) {
            return true;
        }
    }
    return false;
}
