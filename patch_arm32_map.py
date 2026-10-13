import sys

def patch_file():
    with open('core/arch/arm/arm32/hal_pt_arm32.c', 'r') as f:
        content = f.read()

    search = """static int arm32_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_pt == 0U || paddr == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint32_t l1_idx = aligned_vaddr >> 20;
    uint32_t l2_idx = (aligned_vaddr >> 12) & 0xFF;

    pt_l1_t* l1_table = (pt_l1_t*)(uintptr_t)root_pt;

    if ((l1_table->entries[l1_idx] & 0x3) != ARM32_PT_L1_TYPE_PAGE) {
        phys_addr_t new_l2 = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_l2) return -2;
        pt_l2_t* l2_ptr = (pt_l2_t*)(uintptr_t)new_l2;
        for(int i=0; i<256; i++) l2_ptr->entries[i] = 0;
        l1_table->entries[l1_idx] = (new_l2 & 0xFFFFFC00) | ARM32_PT_L1_TYPE_PAGE;
    }

    pt_l2_t* l2_table = (pt_l2_t*)(uintptr_t)(l1_table->entries[l1_idx] & 0xFFFFFC00);

    uint32_t pte_flags = flags_to_arm32(flags);
    l2_table->entries[l2_idx] = (aligned_paddr & 0xFFFFF000) | pte_flags;

    return 0;
}"""

    replace = """static int arm32_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_pt == 0U || paddr == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint32_t l1_idx = aligned_vaddr >> 20;
    uint32_t l2_idx = (aligned_vaddr >> 12) & 0xFF;

    pt_l1_t* l1_table = (pt_l1_t*)(uintptr_t)root_pt;

    if ((l1_table->entries[l1_idx] & 0x3) != ARM32_PT_L1_TYPE_PAGE) {
        phys_addr_t new_l2 = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_l2) return -2;
        pt_l2_t* l2_ptr = (pt_l2_t*)(uintptr_t)new_l2;

        if ((l1_table->entries[l1_idx] & 0x3) == ARM32_PT_L1_TYPE_SECT) {
            uint32_t sect_base = l1_table->entries[l1_idx] & 0xFFF00000;
            uint32_t sect_flags = l1_table->entries[l1_idx] & 0x000FFFFF;
            uint32_t base_l2_flags = ARM32_PT_L2_TYPE_SMALL;

            if (sect_flags & ARM32_PT_L1_SECT_B) base_l2_flags |= ARM32_PT_L2_B;
            if (sect_flags & ARM32_PT_L1_SECT_C) base_l2_flags |= ARM32_PT_L2_C;

            // Section AP bits -> Small Page AP bits
            if (sect_flags & ARM32_PT_L1_SECT_AP0) {
                if ((sect_flags & ARM32_PT_L1_SECT_AP0) && !(sect_flags & (1<<15))) {
                    base_l2_flags |= ARM32_PT_L2_AP0; // Priv R/W
                } else if ((sect_flags & ARM32_PT_L1_SECT_AP0) && (sect_flags & (1<<15))) {
                    base_l2_flags |= ARM32_PT_L2_AP2 | ARM32_PT_L2_AP0; // Priv RO
                }
            } else {
                 base_l2_flags |= ARM32_PT_L2_AP0; // Default Priv R/W
            }
            if (sect_flags & (1 << 4)) {
                base_l2_flags |= ARM32_PT_L2_XN; // XN bit
            }
            if (sect_flags & (1 << 17)) {
                base_l2_flags |= ARM32_PT_L2_nG; // nG bit
            }
            if (sect_flags & (1 << 16)) {
                base_l2_flags |= ARM32_PT_L2_S; // S bit
            }

            for(int i=0; i<256; i++) {
                l2_ptr->entries[i] = (sect_base + (i << 12)) | base_l2_flags;
            }
        } else {
            for(int i=0; i<256; i++) l2_ptr->entries[i] = 0;
        }

        l1_table->entries[l1_idx] = (new_l2 & 0xFFFFFC00) | ARM32_PT_L1_TYPE_PAGE;
    }

    pt_l2_t* l2_table = (pt_l2_t*)(uintptr_t)(l1_table->entries[l1_idx] & 0xFFFFFC00);

    uint32_t pte_flags = flags_to_arm32(flags);
    l2_table->entries[l2_idx] = (aligned_paddr & 0xFFFFF000) | pte_flags;

    return 0;
}"""

    if search in content:
        content = content.replace(search, replace)
        with open('core/arch/arm/arm32/hal_pt_arm32.c', 'w') as f:
            f.write(content)
        print("Success")
    else:
        print("Search string not found")

patch_file()
