import sys

def patch_file():
    with open('core/arch/riscv/riscv32/hal_pt_riscv32.c', 'r') as f:
        content = f.read()

    search = """static int riscv32_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t vpn1 = (aligned_vaddr >> 22) & 0x3FF;
    uint32_t vpn0 = (aligned_vaddr >> 12) & 0x3FF;

    pt_t* l1_table = (pt_t*)(uintptr_t)root_pt;
    if ((l1_table->entries[vpn1] & RISCV32_PT_V) == 0) return -2;
    pt_t* l0_table = (pt_t*)(uintptr_t)((l1_table->entries[vpn1] >> 10) << 12);

    if ((l0_table->entries[vpn0] & RISCV32_PT_V) == 0) return -2;

    if (unmapped_paddr) {
        *unmapped_paddr = (l0_table->entries[vpn0] >> 10) << 12;
    }

    l0_table->entries[vpn0] = 0;

    if (table_empty(l0_table)) {
        mm_free_page((l1_table->entries[vpn1] >> 10) << 12);
        l1_table->entries[vpn1] = 0;
    }

    return 0;
}

static int riscv32_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t vpn1 = (aligned_vaddr >> 22) & 0x3FF;
    uint32_t vpn0 = (aligned_vaddr >> 12) & 0x3FF;

    pt_t* l1_table = (pt_t*)(uintptr_t)root_pt;
    if ((l1_table->entries[vpn1] & RISCV32_PT_V) == 0) return -2;
    pt_t* l0_table = (pt_t*)(uintptr_t)((l1_table->entries[vpn1] >> 10) << 12);

    if ((l0_table->entries[vpn0] & RISCV32_PT_V) == 0) return -2;

    uint32_t paddr = (l0_table->entries[vpn0] >> 10) << 12;
    uint32_t pte_flags = flags_to_riscv(new_flags);

    l0_table->entries[vpn0] = ((paddr >> 12) << 10) | pte_flags;

    return 0;
}

static int riscv32_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t vpn1 = (aligned_vaddr >> 22) & 0x3FF;
    uint32_t vpn0 = (aligned_vaddr >> 12) & 0x3FF;

    pt_t* l1_table = (pt_t*)(uintptr_t)root_pt;
    if ((l1_table->entries[vpn1] & RISCV32_PT_V) == 0) return -2;
    pt_t* l0_table = (pt_t*)(uintptr_t)((l1_table->entries[vpn1] >> 10) << 12);

    if ((l0_table->entries[vpn0] & RISCV32_PT_V) == 0) return -2;

    if (paddr) *paddr = (l0_table->entries[vpn0] >> 10) << 12;
    if (flags) *flags = riscv_to_flags(l0_table->entries[vpn0] & ~RISCV32_PAGE_MASK);

    return 0;
}"""

    replace = """static int riscv32_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t vpn1 = (aligned_vaddr >> 22) & 0x3FF;
    uint32_t vpn0 = (aligned_vaddr >> 12) & 0x3FF;

    pt_t* l1_table = (pt_t*)(uintptr_t)root_pt;
    if ((l1_table->entries[vpn1] & RISCV32_PT_V) == 0) return -2;

    if ((l1_table->entries[vpn1] & (RISCV32_PT_R | RISCV32_PT_W | RISCV32_PT_X)) != 0) {
        if (unmapped_paddr) {
            *unmapped_paddr = ((l1_table->entries[vpn1] >> 10) << 12) | (vpn0 << 12);
        }
        return -2;
    }

    pt_t* l0_table = (pt_t*)(uintptr_t)((l1_table->entries[vpn1] >> 10) << 12);

    if ((l0_table->entries[vpn0] & RISCV32_PT_V) == 0) return -2;

    if (unmapped_paddr) {
        *unmapped_paddr = (l0_table->entries[vpn0] >> 10) << 12;
    }

    l0_table->entries[vpn0] = 0;

    if (table_empty(l0_table)) {
        mm_free_page((l1_table->entries[vpn1] >> 10) << 12);
        l1_table->entries[vpn1] = 0;
    }

    return 0;
}

static int riscv32_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t vpn1 = (aligned_vaddr >> 22) & 0x3FF;
    uint32_t vpn0 = (aligned_vaddr >> 12) & 0x3FF;

    pt_t* l1_table = (pt_t*)(uintptr_t)root_pt;
    if ((l1_table->entries[vpn1] & RISCV32_PT_V) == 0) return -2;

    if ((l1_table->entries[vpn1] & (RISCV32_PT_R | RISCV32_PT_W | RISCV32_PT_X)) != 0) {
        return -2;
    }

    pt_t* l0_table = (pt_t*)(uintptr_t)((l1_table->entries[vpn1] >> 10) << 12);

    if ((l0_table->entries[vpn0] & RISCV32_PT_V) == 0) return -2;

    uint32_t paddr = (l0_table->entries[vpn0] >> 10) << 12;
    uint32_t pte_flags = flags_to_riscv(new_flags);

    if ((pte_flags & RISCV32_PT_W) && !(pte_flags & RISCV32_PT_R)) {
        return -2;
    }

    l0_table->entries[vpn0] = ((paddr >> 12) << 10) | pte_flags;

    return 0;
}

static int riscv32_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    if (root_pt == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint32_t vpn1 = (aligned_vaddr >> 22) & 0x3FF;
    uint32_t vpn0 = (aligned_vaddr >> 12) & 0x3FF;

    pt_t* l1_table = (pt_t*)(uintptr_t)root_pt;
    if ((l1_table->entries[vpn1] & RISCV32_PT_V) == 0) return -2;

    if ((l1_table->entries[vpn1] & (RISCV32_PT_R | RISCV32_PT_W | RISCV32_PT_X)) != 0) {
        if (paddr) *paddr = ((l1_table->entries[vpn1] >> 10) << 12) | (vpn0 << 12);
        if (flags) *flags = riscv_to_flags(l1_table->entries[vpn1] & ~RISCV32_PAGE_MASK);
        return 0;
    }

    pt_t* l0_table = (pt_t*)(uintptr_t)((l1_table->entries[vpn1] >> 10) << 12);

    if ((l0_table->entries[vpn0] & RISCV32_PT_V) == 0) return -2;

    if (paddr) *paddr = (l0_table->entries[vpn0] >> 10) << 12;
    if (flags) *flags = riscv_to_flags(l0_table->entries[vpn0] & ~RISCV32_PAGE_MASK);

    return 0;
}"""

    if search in content:
        content = content.replace(search, replace)
        with open('core/arch/riscv/riscv32/hal_pt_riscv32.c', 'w') as f:
            f.write(content)
        print("Success")
    else:
        print("Search string not found")

patch_file()
