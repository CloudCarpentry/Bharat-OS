import sys

def patch_file():
    with open('core/arch/arm/arm32/hal_pt_arm32.c', 'r') as f:
        content = f.read()

    search = """static int arm32_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t l1_idx = aligned_vaddr >> 20;
    uint32_t l2_idx = (aligned_vaddr >> 12) & 0xFF;

    pt_l1_t* l1_table = (pt_l1_t*)(uintptr_t)root_pt;
    if ((l1_table->entries[l1_idx] & 0x3) != ARM32_PT_L1_TYPE_PAGE) return -2;

    pt_l2_t* l2_table = (pt_l2_t*)(uintptr_t)(l1_table->entries[l1_idx] & 0xFFFFFC00);

    if (!arm32_l2_is_small_page(l2_table->entries[l2_idx])) return -2;

    if (unmapped_paddr) {
        *unmapped_paddr = l2_table->entries[l2_idx] & 0xFFFFF000;
    }

    l2_table->entries[l2_idx] = 0;

    if (table_empty(l2_table)) {
        mm_free_page(l1_table->entries[l1_idx] & 0xFFFFFC00);
        l1_table->entries[l1_idx] = 0;
    }

    return 0;
}

static int arm32_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t l1_idx = aligned_vaddr >> 20;
    uint32_t l2_idx = (aligned_vaddr >> 12) & 0xFF;

    pt_l1_t* l1_table = (pt_l1_t*)(uintptr_t)root_pt;
    if ((l1_table->entries[l1_idx] & 0x3) != ARM32_PT_L1_TYPE_PAGE) return -2;

    pt_l2_t* l2_table = (pt_l2_t*)(uintptr_t)(l1_table->entries[l1_idx] & 0xFFFFFC00);

    if (!arm32_l2_is_small_page(l2_table->entries[l2_idx])) return -2;

    uint32_t paddr = l2_table->entries[l2_idx] & 0xFFFFF000;
    uint32_t pte_flags = flags_to_arm32(new_flags);

    l2_table->entries[l2_idx] = paddr | pte_flags;

    return 0;
}

static int arm32_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t l1_idx = aligned_vaddr >> 20;
    uint32_t l2_idx = (aligned_vaddr >> 12) & 0xFF;

    pt_l1_t* l1_table = (pt_l1_t*)(uintptr_t)root_pt;
    if ((l1_table->entries[l1_idx] & 0x3) != ARM32_PT_L1_TYPE_PAGE) return -2;

    pt_l2_t* l2_table = (pt_l2_t*)(uintptr_t)(l1_table->entries[l1_idx] & 0xFFFFFC00);

    if (!arm32_l2_is_small_page(l2_table->entries[l2_idx])) return -2;

    if (paddr) *paddr = l2_table->entries[l2_idx] & 0xFFFFF000;
    if (flags) *flags = arm32_to_flags(l2_table->entries[l2_idx] & ~0xFFFFF000);

    return 0;
}"""

    replace = """static int arm32_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t l1_idx = aligned_vaddr >> 20;
    uint32_t l2_idx = (aligned_vaddr >> 12) & 0xFF;

    pt_l1_t* l1_table = (pt_l1_t*)(uintptr_t)root_pt;
    uint32_t l1_type = l1_table->entries[l1_idx] & 0x3;

    if (l1_type == ARM32_PT_L1_TYPE_FAULT) return -2;

    if (l1_type == ARM32_PT_L1_TYPE_SECT) {
        if (unmapped_paddr) {
            *unmapped_paddr = (l1_table->entries[l1_idx] & 0xFFF00000) | (l2_idx << 12);
        }
        return -2;
    }

    if (l1_type != ARM32_PT_L1_TYPE_PAGE) return -2;

    pt_l2_t* l2_table = (pt_l2_t*)(uintptr_t)(l1_table->entries[l1_idx] & 0xFFFFFC00);

    if (!arm32_l2_is_small_page(l2_table->entries[l2_idx])) return -2;

    if (unmapped_paddr) {
        *unmapped_paddr = l2_table->entries[l2_idx] & 0xFFFFF000;
    }

    l2_table->entries[l2_idx] = 0;

    if (table_empty(l2_table)) {
        mm_free_page(l1_table->entries[l1_idx] & 0xFFFFFC00);
        l1_table->entries[l1_idx] = 0;
    }

    return 0;
}

static int arm32_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t l1_idx = aligned_vaddr >> 20;
    uint32_t l2_idx = (aligned_vaddr >> 12) & 0xFF;

    pt_l1_t* l1_table = (pt_l1_t*)(uintptr_t)root_pt;
    uint32_t l1_type = l1_table->entries[l1_idx] & 0x3;

    if (l1_type == ARM32_PT_L1_TYPE_FAULT) return -2;

    if (l1_type == ARM32_PT_L1_TYPE_SECT) {
        return -2;
    }

    if (l1_type != ARM32_PT_L1_TYPE_PAGE) return -2;

    pt_l2_t* l2_table = (pt_l2_t*)(uintptr_t)(l1_table->entries[l1_idx] & 0xFFFFFC00);

    if (!arm32_l2_is_small_page(l2_table->entries[l2_idx])) return -2;

    uint32_t paddr = l2_table->entries[l2_idx] & 0xFFFFF000;
    uint32_t pte_flags = flags_to_arm32(new_flags);

    l2_table->entries[l2_idx] = paddr | pte_flags;

    return 0;
}

static int arm32_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t l1_idx = aligned_vaddr >> 20;
    uint32_t l2_idx = (aligned_vaddr >> 12) & 0xFF;

    pt_l1_t* l1_table = (pt_l1_t*)(uintptr_t)root_pt;
    uint32_t l1_type = l1_table->entries[l1_idx] & 0x3;

    if (l1_type == ARM32_PT_L1_TYPE_FAULT) return -2;

    if (l1_type == ARM32_PT_L1_TYPE_SECT) {
        if (paddr) *paddr = (l1_table->entries[l1_idx] & 0xFFF00000) | (l2_idx << 12);
        if (flags) {
            uint32_t sect_flags = l1_table->entries[l1_idx] & 0x000FFFFF;
            uint32_t hal_flags = HAL_PT_FLAG_READ;
            if (sect_flags & ARM32_PT_L1_SECT_AP0) {
                if ((sect_flags & ARM32_PT_L1_SECT_AP0) && !(sect_flags & (1<<15))) {
                    hal_flags |= HAL_PT_FLAG_WRITE;
                }
            } else {
                 hal_flags |= HAL_PT_FLAG_WRITE;
            }
            if (!(sect_flags & (ARM32_PT_L1_SECT_B | ARM32_PT_L1_SECT_C))) {
                hal_flags |= HAL_PT_FLAG_DEVICE;
            }
            hal_flags |= HAL_PT_FLAG_EXEC;
            if (sect_flags & (1 << 4)) {
                hal_flags &= ~HAL_PT_FLAG_EXEC;
            }
            hal_flags |= HAL_PT_FLAG_GLOBAL;
            *flags = hal_flags;
        }
        return 0;
    }

    if (l1_type != ARM32_PT_L1_TYPE_PAGE) return -2;

    pt_l2_t* l2_table = (pt_l2_t*)(uintptr_t)(l1_table->entries[l1_idx] & 0xFFFFFC00);

    if (!arm32_l2_is_small_page(l2_table->entries[l2_idx])) return -2;

    if (paddr) *paddr = l2_table->entries[l2_idx] & 0xFFFFF000;
    if (flags) *flags = arm32_to_flags(l2_table->entries[l2_idx] & ~0xFFFFF000);

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
