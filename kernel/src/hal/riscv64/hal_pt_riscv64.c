#include "../../../include/hal/hal_pt.h"
#include "../../../include/hal/hal_tlb.h"

phys_addr_t riscv64_pt_create_address_space(phys_addr_t kernel_root_table) {
    (void)kernel_root_table;
    return 0; // Stub
}

void riscv64_pt_destroy_address_space(phys_addr_t root_pt) {
    (void)root_pt; // Stub
}

int riscv64_pt_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)root_pt; (void)vaddr; (void)paddr; (void)flags;
    return -1; // Stub
}

int riscv64_pt_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    (void)root_pt; (void)vaddr;
    if (unmapped_paddr) *unmapped_paddr = 0;
    return -1; // Stub
}

int riscv64_pt_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    (void)root_pt; (void)vaddr; (void)new_flags;
    return -1; // Stub
}

int riscv64_pt_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    (void)root_pt; (void)vaddr;
    if (paddr) *paddr = 0;
    if (flags) *flags = 0;
    return -1; // Stub
}

hal_pt_ops_t riscv64_hal_pt_ops = {
    .create_address_space  = riscv64_pt_create_address_space,
    .destroy_address_space = riscv64_pt_destroy_address_space,
    .map_page              = riscv64_pt_map_page,
    .unmap_page            = riscv64_pt_unmap_page,
    .protect_page          = riscv64_pt_protect_page,
    .query_page            = riscv64_pt_query_page,
};

static void riscv64_tlb_flush_page_local(virt_addr_t vaddr) {
    (void)vaddr; // Stub
}

static void riscv64_tlb_flush_all_local(void) {
    // Stub
}

static void riscv64_tlb_flush_asid_local(uint16_t asid) {
    (void)asid; // Stub
}

static void riscv64_tlb_flush_page_remote(uint16_t target_core, uint16_t asid, virt_addr_t vaddr) {
    (void)target_core; (void)asid; (void)vaddr; // Stub
}

static void riscv64_tlb_flush_all_remote(uint16_t target_core, uint16_t asid) {
    (void)target_core; (void)asid; // Stub
}

static void riscv64_tlb_flush_page_broadcast(uint16_t asid, virt_addr_t vaddr) {
    (void)asid; (void)vaddr; // Stub
}

static void riscv64_tlb_flush_all_broadcast(uint16_t asid) {
    (void)asid; // Stub
}

hal_tlb_ops_t riscv64_hal_tlb_ops = {
    .flush_page_local      = riscv64_tlb_flush_page_local,
    .flush_all_local       = riscv64_tlb_flush_all_local,
    .flush_asid_local      = riscv64_tlb_flush_asid_local,
    .flush_page_remote     = riscv64_tlb_flush_page_remote,
    .flush_all_remote      = riscv64_tlb_flush_all_remote,
    .flush_page_broadcast  = riscv64_tlb_flush_page_broadcast,
    .flush_all_broadcast   = riscv64_tlb_flush_all_broadcast,
};
