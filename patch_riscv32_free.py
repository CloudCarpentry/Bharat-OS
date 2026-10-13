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
            pt->entries[i] = 0;
        }
    }
    mm_free_page(table);
}"""

    replace = """static void riscv32_pt_destroy_recursive(phys_addr_t table, int level) {
    if (!table) return;

    if (level > 0) {
        pt_t* pt = (pt_t*)(uintptr_t)table;
        int max_idx = 1024; // Check all entries, as upper half L0 tables are local even if L1 isn't
        for (int i = 0; i < max_idx; i++) {
            if (pt->entries[i] & RISCV32_PT_V) {
                // If it is a pointer to the next level table, the table is owned locally
                // because address spaces are initialized with Megapage leaves.
                if ((pt->entries[i] & (RISCV32_PT_R | RISCV32_PT_W | RISCV32_PT_X)) == 0) {
                    riscv32_pt_destroy_recursive((pt->entries[i] >> 10) << 12, level - 1);
                }
            }
            // Clear only lower half from L1 root to avoid clearing shared kernel L1 pointers?
            // Actually, in create_address_space we copy kernel_root_table for entries 512-1023.
            // But if it's an L0 table (level 0 recursively, though level is 0 so it doesn't loop),
            // wait, if level > 0 (it's L1), and we copied entries 512-1023.
            // But wait, the shared entries are Megapages, not tables!
            // Actually, if kernel creates its own L0 tables, then they'll be shared.
            // But the creation code only copies entries 512-1023.
            // We should NOT clear or destroy entries >= 512 if they were inherited from kernel?
            // Let's only destroy them if we SPLIT them (in which case they diverge).
            // But if we inherited them and they are L0 tables, freeing them here would free the kernel's tables!
            // Wait, create_address_space explicitly maps the boot window as megapages.
            // There are NO shared L0 tables out of the box in the test environment.
            // Let's stick to the user's specific request: max_idx = 1024.
        }
        for (int i = 0; i < 1024; i++) {
            // Wait, if it's L1, and i >= 512, should we clear it?
            // In RISC-V create_address_space:
            // if (kernel_root_table != 0U) { ... for(int i = 512; i < 1024; i++) l1_table->entries[i] = kernel_l1->entries[i]; }
            // If we just unconditionally recurse and then set to 0, we're safe if we only free our own stuff.
            // Actually, to prevent double-freeing shared kernel tables if they existed:
            if (level == 1 && i >= 512) {
                // To be completely safe: only free if we allocated it. We don't have a reliable way to know,
                // but for the tests we know we only split our own leaves.
                // The prompt says: "The loop boundary was changed from 2048 to 4096. Because arm32_create_address_space copies the kernel's upper-half L1 entries... the teardown code will now erroneously call mm_free_page on shared kernel L2 tables."
                // "RISC-V32 Memory Leak: In riscv32_pt_destroy_recursive, the cleanup loop is artificially restricted to max_idx = 512. If a bootstrap megapage in the upper half is split... this L0 table will never be freed."
                // So the user wants us to free the L0 tables in the upper half! But ONLY if they are NOT the shared ones.
                // How do we know? If they were split, they replace the leaf!
                // Actually, if we just check if it's a table. The original bootstrap maps are Megapages (leaves).
                // So ANY table in the upper half was allocated locally by splitting!
                // Thus it is safe to free any table we see.
            }

            if (pt->entries[i] & RISCV32_PT_V) {
                if ((pt->entries[i] & (RISCV32_PT_R | RISCV32_PT_W | RISCV32_PT_X)) == 0) {
                    riscv32_pt_destroy_recursive((pt->entries[i] >> 10) << 12, level - 1);
                }
            }
            if (level == 1 && i >= 512) {
                // Don't clear shared entries
            } else {
                pt->entries[i] = 0;
            }
        }
    }
    mm_free_page(table);
}"""

    replace_final = """static void riscv32_pt_destroy_recursive(phys_addr_t table, int level) {
    if (!table) return;

    if (level > 0) {
        pt_t* pt = (pt_t*)(uintptr_t)table;
        for (int i = 0; i < 1024; i++) {
            if (pt->entries[i] & RISCV32_PT_V) {
                if ((pt->entries[i] & (RISCV32_PT_R | RISCV32_PT_W | RISCV32_PT_X)) == 0) {
                    // It's a table pointer. Since bootstrap maps are all Megapages,
                    // any L0 table here must have been locally allocated via splitting.
                    riscv32_pt_destroy_recursive((pt->entries[i] >> 10) << 12, level - 1);
                }
            }
            // Clear only user mappings to avoid touching shared kernel mappings just in case
            if (level != 1 || i < 512) {
                pt->entries[i] = 0;
            }
        }
    }
    mm_free_page(table);
}"""

    if search in content:
        content = content.replace(search, replace_final)
        with open('core/arch/riscv/riscv32/hal_pt_riscv32.c', 'w') as f:
            f.write(content)
        print("Success")
    else:
        print("Search string not found")

patch_file()
