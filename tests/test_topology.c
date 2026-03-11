#include <stdio.h>
#include <assert.h>
#include "hal/hal_topology.h"
#include "hal/hal_boot.h"

static bharat_boot_info_t mock_boot_info = {
    .cpu_count = 4,
    .cpus = { { .cpu_id = 0 }, { .cpu_id = 1 }, { .cpu_id = 2 }, { .cpu_id = 3 } },
    .mem_region_count = 2,
    .mem_regions = {
        { .base = 0x0, .size = 0x10000000 },
        { .base = 0x10000000, .size = 0x10000000 }
    }
};

// We will mock hal_boot_get_info to return this mock info
bharat_boot_info_t* hal_boot_get_info(void) {
    return &mock_boot_info;
}

int main(void) {
    printf("[TEST] Running Topology UMA Fallback Tests...\n");

    // Init with no NUMA info should fallback to UMA
    hal_topology_init();

    assert(hal_topology_is_numa() == false);

    assert(hal_topology_get_cpu_node(0) == 0);
    assert(hal_topology_get_cpu_node(3) == 0);

    assert(hal_topology_get_mem_node(0x0) == 0);
    assert(hal_topology_get_mem_node(0x15000000) == 0);

    // Now inject some fake NUMA info
    mock_boot_info.mem_regions[1].numa_node = 1;
    assert(hal_topology_is_numa() == true);
    assert(hal_topology_get_mem_node(0x15000000) == 1);

    printf("[TEST] Topology UMA Fallback Passed.\n");
    return 0;
}
