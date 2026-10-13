import sys

def patch_file():
    with open('core/arch/riscv/riscv32/hal_pt_riscv32.c', 'r') as f:
        content = f.read()

    search = """static void riscv32_pt_destroy_recursive(phys_addr_t table, int level) {
    if (!table) return;

    if (level > 0) {
        pt_t* pt = (pt_t*)(uintptr_t)table;
        int max_idx = (level == 1) ? 512 : 1024; // Free only bottom half of L1
        for (int i = 0; i < max_idx; i++) {
            if (pt->entries[i] & RISCV32_PT_V) {
                if ((pt->entries[i] & (RISCV32_PT_R | RISCV32_PT_W | RISCV32_PT_X)) == 0) {
                    riscv32_pt_destroy_recursive((pt->entries[i] >> 10) << 12, level - 1);
                }
            }
        }
    }
    mm_free_page(table);
}

static void riscv32_destroy_address_space(phys_addr_t root_pt) {
    if (root_pt) {
        riscv32_pt_destroy_recursive(root_pt, 1);
    }
}

static int riscv32_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_pt == 0U || paddr == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint32_t vpn1 = (aligned_vaddr >> 22) & 0x3FF;
    uint32_t vpn0 = (aligned_vaddr >> 12) & 0x3FF;

    pt_t* l1_table = (pt_t*)(uintptr_t)root_pt;

    uint32_t table_flags = RISCV32_PT_V;

    if ((l1_table->entries[vpn1] & RISCV32_PT_V) == 0) {
        phys_addr_t new_l0 = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_l0) return -2;
        pt_t* l0_ptr = (pt_t*)(uintptr_t)new_l0;
        for(int i=0; i<1024; i++) l0_ptr->entries[i] = 0;
        l1_table->entries[vpn1] = ((new_l0 >> 12) << 10) | table_flags;
    } else if ((l1_table->entries[vpn1] &
                (RISCV32_PT_R | RISCV32_PT_W | RISCV32_PT_X)) != 0U) {
        uint32_t leaf = l1_table->entries[vpn1];
        phys_addr_t mega_base = (phys_addr_t)((leaf >> 10) << 12);
        phys_addr_t new_l0 = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_l0) return -2;
        pt_t *l0_ptr = (pt_t *)(uintptr_t)new_l0;
        uint32_t leaf_flags = leaf & 0x3FFU;
        for (uint32_t i = 0; i < 1024U; ++i) {
            phys_addr_t page_base = mega_base + (i << 12);
            l0_ptr->entries[i] = ((page_base >> 12) << 10) | leaf_flags;
        }
        l1_table->entries[vpn1] = ((new_l0 >> 12) << 10) | table_flags;
    }

    pt_t* l0_table = (pt_t*)(uintptr_t)((l1_table->entries[vpn1] >> 10) << 12);

    uint32_t pte_flags = flags_to_riscv(flags);
    l0_table->entries[vpn0] = ((aligned_paddr >> 12) << 10) | pte_flags;

    return 0;
}"""

    replace = """static void riscv32_pt_destroy_recursive(phys_addr_t table, int level) {
    if (!table) return;

    if (level > 0) {
        pt_t* pt = (pt_t*)(uintptr_t)table;
        int max_idx = (level == 1) ? 512 : 1024; // Free only bottom half of L1
        for (int i = 0; i < max_idx; i++) {
            if (pt->entries[i] & RISCV32_PT_V) {
                if ((pt->entries[i] & (RISCV32_PT_R | RISCV32_PT_W | RISCV32_PT_X)) == 0) {
                    riscv32_pt_destroy_recursive((pt->entries[i] >> 10) << 12, level - 1);
                }
            }
            pt->entries[i] = 0;
        }
    }
    mm_free_page(table);
}

static void riscv32_destroy_address_space(phys_addr_t root_pt) {
    if (root_pt) {
        riscv32_pt_destroy_recursive(root_pt, 1);
    }
}

static int riscv32_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_pt == 0U || paddr == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint32_t vpn1 = (aligned_vaddr >> 22) & 0x3FF;
    uint32_t vpn0 = (aligned_vaddr >> 12) & 0x3FF;

    pt_t* l1_table = (pt_t*)(uintptr_t)root_pt;

    uint32_t table_flags = RISCV32_PT_V;

    if ((l1_table->entries[vpn1] & RISCV32_PT_V) == 0) {
        phys_addr_t new_l0 = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_l0) return -2;
        pt_t* l0_ptr = (pt_t*)(uintptr_t)new_l0;
        for(int i=0; i<1024; i++) l0_ptr->entries[i] = 0;
        l1_table->entries[vpn1] = ((new_l0 >> 12) << 10) | table_flags;
    } else if ((l1_table->entries[vpn1] &
                (RISCV32_PT_R | RISCV32_PT_W | RISCV32_PT_X)) != 0U) {
        uint32_t leaf = l1_table->entries[vpn1];
        phys_addr_t mega_base = (phys_addr_t)((leaf >> 10) << 12);
        phys_addr_t new_l0 = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_l0) return -2;
        pt_t *l0_ptr = (pt_t *)(uintptr_t)new_l0;
        uint32_t leaf_flags = leaf & 0x3FFU;
        for (uint32_t i = 0; i < 1024U; ++i) {
            phys_addr_t page_base = mega_base + (i << 12);
            l0_ptr->entries[i] = ((page_base >> 12) << 10) | leaf_flags;
        }
        l1_table->entries[vpn1] = ((new_l0 >> 12) << 10) | table_flags;
    }

    pt_t* l0_table = (pt_t*)(uintptr_t)((l1_table->entries[vpn1] >> 10) << 12);

    uint32_t pte_flags = flags_to_riscv(flags);
    // W=1 R=0 is reserved and invalid in Sv32
    if ((pte_flags & RISCV32_PT_W) && !(pte_flags & RISCV32_PT_R)) {
        return -2;
    }

    l0_table->entries[vpn0] = ((aligned_paddr >> 12) << 10) | pte_flags;

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
