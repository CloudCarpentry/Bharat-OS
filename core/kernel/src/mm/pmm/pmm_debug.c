#include "mm/pmm.h"
#include "mm.h"
#include "console/console_core.h"
#include "bharat/cpu_local.h"

// Expose internal state needed for dump
extern numa_node_t numa_nodes[];
extern uint32_t active_numa_nodes;

void pmm_dump_stats(void) {
    console_write_raw("--- PMM Status ---\n", 19);

    for (uint32_t i = 0; i < active_numa_nodes; i++) {
        // Output basic stats using generic formatting or raw output
        console_write_raw("Node ", 5);
        char c = '0' + i;
        console_write_raw(&c, 1);
        console_write_raw("\n", 1);

        // Count pages in different states
        uint64_t free = 0, alloc = 0, resv = 0, pinned = 0;

        phys_addr_t curr = numa_nodes[i].start_addr;
        phys_addr_t end = curr + (numa_nodes[i].total_pages * PAGE_SIZE);

        while (curr < end) {
            page_t *p = phys_to_page(curr);
            if (p) {
                if (p->state == PMM_PAGE_STATE_FREE) free++;
                else if (p->state == PMM_PAGE_STATE_ALLOCATED) alloc++;
                else if (p->state == PMM_PAGE_STATE_RESERVED) resv++;

                if (p->pin_count > 0) pinned++;
            }
            curr += PAGE_SIZE;
        }

        // This is a minimal textual dump for debugging
        // Normally you would use snprintf here.
    }
}
