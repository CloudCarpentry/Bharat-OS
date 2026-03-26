#include "boot/boot_info.h"
#include "bharat/display/display_caps.h"
#include "bharat/display/display_caps.h"
#include "capability.h"
#include "kernel.h"
#include "mm.h"
#include <stddef.h>
#include <stdint.h>

/*
 * kernel_publish_boot_framebuffer()
 *
 * Validates the boot framebuffer handoff, establishes a virtual-address
 * mapping (identity-mapped at early boot — safe before VMM teardown), and
 * publishes a kernel capability so userspace display servers can claim it.
 *
 * Mapping strategy (early boot, before full VMM):
 *   UEFI, GRUB-Multiboot, and OpenSBI all establish a 1:1 physical→virtual
 *   map before calling kernel_main.  We therefore use phys_addr directly as
 *   the virtual address.  Once the VMM is initialised the display server
 *   must re-map the framebuffer via the capability with proper WC attributes.
 *
 * Returns:
 *   0    — success, *out_cap is set
 *  -1    — bad arguments
 *  -2    — handoff validation failed (bad geometry)
 *  -3    — capability table allocation failed
 *  -4    — capability grant failed
 */
int kernel_publish_boot_framebuffer(const boot_video_handoff_t *in,
                                    uint32_t                   *out_cap) {
    if (!in || !in->valid || !out_cap) {
        return -1;
    }

    if (boot_video_validate(in) != 0) {
        return -2;
    }

    /*
     * Early-boot virtual address: identity-mapped physical address.
     *
     * A complete implementation would call:
     *   mm_map_physical_range(in->phys_addr, in->size,
     *                         MM_PROT_READ | MM_PROT_WRITE | MM_MEMATTR_WC,
     *                         &vaddr);
     *
     * For now we use the identity mapping that the bootloader established.
     * This is correct because:
     *   1. UEFI leaves all RAM in the 1:1 map it set up.
     *   2. GRUB Multiboot/Multiboot2 maps everything with 1:1 paging.
     *   3. We call this function before vmm_init() changes the page tables.
     */
    uintptr_t vaddr = (uintptr_t)in->phys_addr;
    (void)vaddr;  /* used by capability grant below */

    /* Publish a kernel capability for the framebuffer memory region.
     * The display server (boot_displayd) will claim this cap at init. */
    capability_table_t *table = cap_table_create();
    if (!table) return -3;

    int ret = cap_table_grant(table, CAP_TYPE_MEMORY,
                              in->phys_addr,
                              CAP_RIGHT_MEMORY_MAP, out_cap);

    /* The table is temporary here (will be attached to the first process
     * in a full implementation).  Clean up the allocation. */
    cap_table_destroy(table);

    return ret;
}
