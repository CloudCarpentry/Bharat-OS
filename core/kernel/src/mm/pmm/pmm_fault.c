#include "pmm_internal.h"

/*
 * PMM Fault & Physical Address Validation Helpers
 */

bool pmm_is_valid_phys_addr(phys_addr_t phys) {
    for (uint32_t i = 0; i < active_numa_nodes; ++i) {
        numa_node_t *node = &numa_nodes[i];
        phys_addr_t end = node->start_addr + (node->total_pages * PAGE_SIZE);
        if (phys >= node->start_addr && phys < end) {
            return true;
        }
    }
    return false;
}

int pmm_fault_verify_page(phys_addr_t phys, uint32_t access_flags) {
    (void)access_flags;
    page_t *page = phys_to_page(phys);
    if (!page) {
        return -1;
    }
    if (page->state == PMM_PAGE_STATE_FREE) {
        return -2;
    }
    return 0;
}
