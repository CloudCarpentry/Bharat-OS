#include "../../include/mm/vm_object.h"
#include "../../include/mm/pmm.h"
#include "../../include/mm/aspace.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/hal/hal_tlb.h"

// VM Object Backend - Anonymous Memory

static int anon_fault(vm_object_t *obj, address_space_t *aspace, virt_addr_t fault_addr, uint64_t offset, uint32_t flags) {
    if (!obj || !aspace || !active_hal_pt) return -1;

    virt_addr_t aligned_vaddr = fault_addr & ~(PAGE_SIZE - 1U);
    mmu_flags_t mmu_flags = MMU_USER;

    // Zero-fill-on-demand for anonymous memory
    phys_addr_t paddr = 0;

    // Check if handling COW fault
    if (flags & CAP_RIGHT_WRITE) {
        phys_addr_t existing_paddr = 0;
        mmu_flags_t existing_flags = 0;

        int query_res = active_hal_pt->query_page(aspace->root_pt, aligned_vaddr, &existing_paddr, &existing_flags);
        if (query_res == 0 && (existing_flags & MMU_COW)) {
            // Break COW
            paddr = mm_alloc_page(NUMA_NODE_ANY);
            if (!paddr) return -2;

            uint8_t *src = (uint8_t *)P2V(existing_paddr);
            uint8_t *dst = (uint8_t *)P2V(paddr);
            __builtin_memcpy(dst, src, PAGE_SIZE);

            mm_free_page(existing_paddr);

            mmu_flags |= MMU_WRITE;
            int ret = active_hal_pt->map_page(aspace->root_pt, aligned_vaddr, paddr, mmu_flags);
            hal_tlb_invalidate_page(aspace, aligned_vaddr);
            return ret;
        }
    }

    // Standard zero-fill allocation
    paddr = mm_alloc_page(NUMA_NODE_ANY);
    if (!paddr) return -3;

    uint8_t *dst = (uint8_t *)P2V(paddr);
    __builtin_memset(dst, 0, PAGE_SIZE);

    if (flags & CAP_RIGHT_WRITE) mmu_flags |= MMU_WRITE;
    if (flags & CAP_RIGHT_EXECUTE) mmu_flags |= MMU_EXECUTE;

    int ret = active_hal_pt->map_page(aspace->root_pt, aligned_vaddr, paddr, mmu_flags);
    if (ret == 0) {
        __atomic_add_fetch(&obj->resident_pages, 1, __ATOMIC_SEQ_CST);
        hal_tlb_invalidate_page(aspace, aligned_vaddr);
    } else {
        mm_free_page(paddr);
    }

    return ret;
}

static void anon_destroy(vm_object_t *obj) {
    // Release cache or resident pages
    // Simplified: in a real OS, we iterate the object's page cache array and free
    if (obj) {
        // Free object structure
    }
}

static vm_object_ops_t anon_ops = {
    .fault = anon_fault,
    .destroy = anon_destroy,
};

vm_object_t *vm_object_create_anon(uint64_t size) {
    // Allocate object metadata (for now simplified static allocation or kmem)
    static vm_object_t static_anon_pool[128];
    static int pool_idx = 0;

    vm_object_t *obj = &static_anon_pool[pool_idx++];
    obj->size = size;
    obj->refcount = 1;
    obj->resident_pages = 0;
    obj->type = VM_OBJECT_ANON;
    obj->ops = &anon_ops;
    return obj;
}

vm_object_t *vm_object_create_shared(uint64_t size) {
    return vm_object_create_anon(size); // Placeholder
}

vm_object_t *vm_object_create_file(struct vfs_node *node, uint64_t size) {
    (void)node; (void)size;
    return NULL; // Stubbed for now
}
