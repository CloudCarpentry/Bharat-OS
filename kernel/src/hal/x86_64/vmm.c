#include "../../../include/hal/vmm.h"
#include "../../../include/numa.h"

#define P2V(x) ((void*)(uintptr_t)(x))
#define V2P(x) ((phys_addr_t)(uintptr_t)(x))

#define VMM_FLAG_PRESENT 0x1U

typedef struct {
    uint64_t entries[512];
} pt_t, pd_t, pdp_t, pml4_t;

static virt_addr_t align_down(virt_addr_t value) {
    return value & ~(virt_addr_t)(PAGE_SIZE - 1U);
}

phys_addr_t hal_vmm_init_root(void) {
    phys_addr_t root_dir_phys = mm_alloc_page(NUMA_NODE_ANY);
    if (root_dir_phys == 0U) {
        return 0;
    }

    pml4_t* pml4 = (pml4_t*)P2V(root_dir_phys);
    for (int i = 0; i < 512; i++) {
        pml4->entries[i] = 0;
    }

    return root_dir_phys;
}

int hal_vmm_map_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_table == 0U || paddr == 0U) {
        return -1;
    }

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pml4_t* pml4 = (pml4_t*)P2V(root_table);
    if ((pml4->entries[pml4_idx] & VMM_FLAG_PRESENT) == 0) {
        phys_addr_t new_pdp = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pdp) return -2;
        pdp_t* pdp_ptr = (pdp_t*)P2V(new_pdp);
        for(int i=0; i<512; i++) pdp_ptr->entries[i] = 0;
        pml4->entries[pml4_idx] = new_pdp | VMM_FLAG_PRESENT | (flags & PAGE_USER) | CAP_RIGHT_WRITE;
    }

    pdp_t* pdp = (pdp_t*)P2V(pml4->entries[pml4_idx] & ~0xFFFULL);
    if ((pdp->entries[pdp_idx] & VMM_FLAG_PRESENT) == 0) {
        phys_addr_t new_pd = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pd) return -2;
        pd_t* pd_ptr = (pd_t*)P2V(new_pd);
        for(int i=0; i<512; i++) pd_ptr->entries[i] = 0;
        pdp->entries[pdp_idx] = new_pd | VMM_FLAG_PRESENT | (flags & PAGE_USER) | CAP_RIGHT_WRITE;
    }

    pd_t* pd = (pd_t*)P2V(pdp->entries[pdp_idx] & ~0xFFFULL);
    if ((pd->entries[pd_idx] & VMM_FLAG_PRESENT) == 0) {
        phys_addr_t new_pt = mm_alloc_page(NUMA_NODE_ANY);
        if (!new_pt) return -2;
        pt_t* pt_ptr = (pt_t*)P2V(new_pt);
        for(int i=0; i<512; i++) pt_ptr->entries[i] = 0;
        pd->entries[pd_idx] = new_pt | VMM_FLAG_PRESENT | (flags & PAGE_USER) | CAP_RIGHT_WRITE;
    }

    pt_t* pt = (pt_t*)P2V(pd->entries[pd_idx] & ~0xFFFULL);
    pt->entries[pt_idx] = aligned_paddr | VMM_FLAG_PRESENT | flags;

    return 0;
}

int hal_vmm_unmap_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* unmapped_paddr) {
    if (root_table == 0U) {
        return -1;
    }

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pml4_t* pml4 = (pml4_t*)P2V(root_table);
    if ((pml4->entries[pml4_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pdp_t* pdp = (pdp_t*)P2V(pml4->entries[pml4_idx] & ~0xFFFULL);
    if ((pdp->entries[pdp_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pd_t* pd = (pd_t*)P2V(pdp->entries[pdp_idx] & ~0xFFFULL);
    if ((pd->entries[pd_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pt_t* pt = (pt_t*)P2V(pd->entries[pd_idx] & ~0xFFFULL);

    if (unmapped_paddr) {
        *unmapped_paddr = pt->entries[pt_idx] & ~0xFFFULL;
    }

    pt->entries[pt_idx] = 0;
    return 0;
}

phys_addr_t hal_vmm_setup_address_space(phys_addr_t kernel_root_table) {
    phys_addr_t root = mm_alloc_page(NUMA_NODE_ANY);
    if (root == 0U) {
        return 0;
    }

    pml4_t* pml4 = (pml4_t*)P2V(root);
    for (int i = 0; i < 512; i++) {
        pml4->entries[i] = 0;
    }

    if (kernel_root_table != 0U) {
        pml4_t* kernel_pml4 = (pml4_t*)P2V(kernel_root_table);
        pml4->entries[511] = kernel_pml4->entries[511];
    }

    return root;
}

int hal_vmm_get_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* paddr, uint32_t* flags) {
    if (root_table == 0U) return -1;

    uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (vaddr >> 12) & 0x1FF;

    pml4_t* pml4 = (pml4_t*)P2V(root_table);
    if ((pml4->entries[pml4_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pdp_t* pdp = (pdp_t*)P2V(pml4->entries[pml4_idx] & ~0xFFFULL);
    if ((pdp->entries[pdp_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pd_t* pd = (pd_t*)P2V(pdp->entries[pdp_idx] & ~0xFFFULL);
    if ((pd->entries[pd_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pt_t* pt = (pt_t*)P2V(pd->entries[pd_idx] & ~0xFFFULL);

    uint64_t old_entry = pt->entries[pt_idx];
    if ((old_entry & VMM_FLAG_PRESENT) == 0) return -2;

    if (paddr) *paddr = old_entry & ~0xFFFULL;
    if (flags) *flags = old_entry & 0xFFFULL;

    return 0;
}

int hal_vmm_update_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (root_table == 0U) return -1;

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pml4_t* pml4 = (pml4_t*)P2V(root_table);
    if ((pml4->entries[pml4_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pdp_t* pdp = (pdp_t*)P2V(pml4->entries[pml4_idx] & ~0xFFFULL);
    if ((pdp->entries[pdp_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pd_t* pd = (pd_t*)P2V(pdp->entries[pdp_idx] & ~0xFFFULL);
    if ((pd->entries[pd_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pt_t* pt = (pt_t*)P2V(pd->entries[pd_idx] & ~0xFFFULL);

    pt->entries[pt_idx] = aligned_paddr | VMM_FLAG_PRESENT | flags;
    return 0;
}
