#include "bharat/boot_info.h"
#include "capability.h"
#include "kernel.h"
#include "mm.h"
#include <stddef.h>

int kernel_publish_boot_framebuffer(const boot_video_handoff_t *in, uint32_t *out_cap) {
    if (!in || !in->valid || !out_cap) {
        return -1;
    }

    if (boot_video_validate(in) != 0) {
        return -2;
    }

    // 1. Map physical address
    // In a full implementation we would set up write-combine attributes
    // using mm_map_physical_range, ensuring we only map memory once
    // and explicitly bypass caches.
    uint64_t vaddr = 0;
    // int map_ret = mm_map_physical_range(in->phys_addr, in->size, MM_PROT_READ | MM_PROT_WRITE | MM_MEMATTR_WC, &vaddr);
    // if (map_ret != 0) return -3;

    // For now we assume a basic mapping hook would work.
    (void)vaddr;

    // 2. Publish Capability
    // Use the kernel capability subsystem to generate a generic capability ID
    // that boot_displayd can claim.
    capability_table_t* table = cap_table_create();
    if (!table) return -4;

    int ret = cap_table_grant(table, CAP_OBJ_MEMORY, in->phys_addr, CAP_PERM_MAP, out_cap);

    // Note: in reality we'd attach this to the process or store it globally for the first process
    cap_table_destroy(table);

    return ret;
}
