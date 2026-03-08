#include "../../include/mm.h"
#include "../../include/numa.h"
#include "../../include/hal/hal.h"
#include "../../include/advanced/formal_verif.h"
#include "../../include/sched.h"
#include "../../include/capability.h"

#include <stddef.h>
#include <stdint.h>

// For this architecture-neutral layer without a direct-map base defined in hal.h,
// we assume physical == virtual for the identity mapped kernel space or tests.
#define P2V(x) ((void*)(uintptr_t)(x))
#define V2P(x) ((phys_addr_t)(uintptr_t)(x))

#define VMM_FLAG_PRESENT 0x1U

typedef struct {
    uint64_t entries[512];
} pt_t, pd_t, pdp_t, pml4_t;

static address_space_t kernel_space;

static virt_addr_t align_down(virt_addr_t value) {
    return value & ~(virt_addr_t)(PAGE_SIZE - 1U);
}

int vmm_init(void) {
    phys_addr_t root_dir_phys = mm_alloc_page(NUMA_NODE_ANY);
    if (root_dir_phys == 0U) {
        return -1;
    }

    // Zero the root table
    pml4_t* pml4 = (pml4_t*)P2V(root_dir_phys);
    for (int i = 0; i < 512; i++) {
        pml4->entries[i] = 0;
    }

    kernel_space.root_table = root_dir_phys;
    return 0;
}

int mm_vmm_map_page(address_space_t* as, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (!as || as->root_table == 0U || paddr == 0U) {
        return -1;
    }

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    if ((flags & PAGE_COW) != 0U) {
        mm_inc_page_ref(aligned_paddr);
        flags &= ~CAP_RIGHT_WRITE;
    }

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pml4_t* pml4 = (pml4_t*)P2V(as->root_table);
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

// Global TLB shootdown function
void tlb_shootdown(virt_addr_t vaddr) {
    hal_tlb_flush((unsigned long long)vaddr);

    // IPI based shootdown for cores executing the same process (or system wide for kernel mappings)
    uint32_t current_core = hal_cpu_get_id();
    for (uint32_t i = 0; i < 8; ++i) { // MAX_SUPPORTED_CORES is 8
        if (i != current_core) {
            hal_send_ipi_payload(i, (uint64_t)vaddr);
        }
    }
}

int mm_vmm_unmap_page(address_space_t* as, virt_addr_t vaddr) {
    if (!as || as->root_table == 0U) {
        return -1;
    }

    virt_addr_t aligned_vaddr = align_down(vaddr);

    uint64_t pml4_idx = (aligned_vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (aligned_vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (aligned_vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (aligned_vaddr >> 12) & 0x1FF;

    pml4_t* pml4 = (pml4_t*)P2V(as->root_table);
    if ((pml4->entries[pml4_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pdp_t* pdp = (pdp_t*)P2V(pml4->entries[pml4_idx] & ~0xFFFULL);
    if ((pdp->entries[pdp_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pd_t* pd = (pd_t*)P2V(pdp->entries[pdp_idx] & ~0xFFFULL);
    if ((pd->entries[pd_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pt_t* pt = (pt_t*)P2V(pd->entries[pd_idx] & ~0xFFFULL);

    // Decrement reference count for the physical page being unmapped
    phys_addr_t paddr = pt->entries[pt_idx] & ~0xFFFULL;
    if (paddr != 0) {
        mm_free_page(paddr); // Assuming mm_free_page correctly handles ref counts
    }

    pt->entries[pt_idx] = 0;

    tlb_shootdown(aligned_vaddr);
    return 0;
}

int vmm_map_page(virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    return mm_vmm_map_page(&kernel_space, vaddr, paddr, flags);
}

int vmm_unmap_page(virt_addr_t vaddr) {
    return mm_vmm_unmap_page(&kernel_space, vaddr);
}

int vmm_map_device_mmio(virt_addr_t vaddr, phys_addr_t paddr, capability_t* cap, int is_npu) {
    if (!cap) {
        return -3; // ERR_CAP_DENIED / IPC_ERR_WOULD_BLOCK
    }

    uint32_t required_right = is_npu ? CAP_RIGHT_DEVICE_NPU : CAP_RIGHT_DEVICE_GPU;
    if ((cap->rights_mask & required_right) == 0U) {
        return -3; // ERR_CAP_DENIED
    }

    // Since `cap` is a capability_t (which is a capability_token_t alias), we should verify it
    // against the current process capability table if we had the process context, but here
    // we just check the token's rights directly as instructed by its structure.
    // If the token is passed, we check `rights_mask`.
    // In a fully integrated system we might also check `target_object_id` or similar matches `paddr`.

    return mm_vmm_map_page(&kernel_space, vaddr, paddr, CAP_RIGHT_READ | CAP_RIGHT_WRITE);
}

address_space_t* mm_create_address_space(void) {
    phys_addr_t root = mm_alloc_page(NUMA_NODE_ANY);
    if (root == 0U) {
        return NULL;
    }

    pml4_t* pml4 = (pml4_t*)P2V(root);
    for (int i = 0; i < 512; i++) {
        pml4->entries[i] = 0;
    }

    // Map kernel space into higher half (assuming top entry 511 points to kernel root pdp)
    if (kernel_space.root_table != 0U) {
        pml4_t* kernel_pml4 = (pml4_t*)P2V(kernel_space.root_table);
        pml4->entries[511] = kernel_pml4->entries[511]; // Copy kernel mapping
    }

    address_space_t* as = (address_space_t*)(uintptr_t)mm_alloc_page(NUMA_NODE_ANY);
    if (!as) {
        mm_free_page(root);
        return NULL;
    }

    as->root_table = root;
    return as;
}

// Copy-on-Write Fault Handler
int vmm_handle_cow_fault(address_space_t* as, virt_addr_t vaddr) {
    if (!as || as->root_table == 0U) return -1;

    uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (vaddr >> 12) & 0x1FF;

    pml4_t* pml4 = (pml4_t*)P2V(as->root_table);
    if ((pml4->entries[pml4_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pdp_t* pdp = (pdp_t*)P2V(pml4->entries[pml4_idx] & ~0xFFFULL);
    if ((pdp->entries[pdp_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pd_t* pd = (pd_t*)P2V(pdp->entries[pdp_idx] & ~0xFFFULL);
    if ((pd->entries[pd_idx] & VMM_FLAG_PRESENT) == 0) return -2;
    pt_t* pt = (pt_t*)P2V(pd->entries[pd_idx] & ~0xFFFULL);

    uint64_t old_entry = pt->entries[pt_idx];
    if ((old_entry & VMM_FLAG_PRESENT) == 0) return -2;

    phys_addr_t old_phys = old_entry & ~0xFFFULL;

    // Allocate new page
    phys_addr_t new_phys = mm_alloc_page(NUMA_NODE_ANY);
    if (!new_phys) return -1;

    // Copy data
    uint8_t* src = (uint8_t*)P2V(old_phys);
    uint8_t* dst = (uint8_t*)P2V(new_phys);
    for (int i = 0; i < PAGE_SIZE; i++) {
        dst[i] = src[i];
    }

    // Update mapping with Write permission, clear CoW
    uint64_t flags = (old_entry & 0xFFFULL) | CAP_RIGHT_WRITE;
    flags &= ~PAGE_COW;
    pt->entries[pt_idx] = new_phys | flags;

    // Decrement old page reference
    mm_free_page(old_phys);

    tlb_shootdown(vaddr);
    return 0;
}
