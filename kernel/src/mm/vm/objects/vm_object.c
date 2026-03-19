#include "../../include/mm/vm_object.h"
#include "../../include/mm/pmm.h"
#include "../../include/mm/aspace.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/hal/hal_tlb.h"

// VM Object Backend - Anonymous Memory

static int anon_fault(struct vm_object *obj, struct vm_region *region, uintptr_t fault_addr, uint32_t access, phys_addr_t *out_page, uint32_t *out_page_flags) {
    if (!obj) return -1;
    (void)region;
    (void)fault_addr;

    phys_addr_t paddr = mm_alloc_page(NUMA_NODE_ANY);
    if (!paddr) return VM_FAULT_OOM;

    uint8_t *dst = (uint8_t *)paddr; // Assuming identity map for now
    __builtin_memset(dst, 0, PAGE_SIZE);

    if (out_page) *out_page = paddr;
    if (out_page_flags) *out_page_flags = access; // Needs proper flags mapping

    return VM_FAULT_HANDLED;
}

static void anon_destroy(struct vm_object *obj) {
    if (obj) {
        // Free object structure
    }
}

static vm_object_ops_t anon_ops = {
    .fault = anon_fault,
    .release = anon_destroy,
};

vm_object_t *vm_object_create_anon(size_t size, uint32_t flags) {
    static vm_object_t static_anon_pool[128];
    static int pool_idx = 0;

    vm_object_t *obj = &static_anon_pool[pool_idx++];
    obj->size = size;
    obj->refcount = 1;
    obj->kind = VM_OBJECT_ANON;
    obj->ops = &anon_ops;
    obj->flags = flags;
    return obj;
}

vm_object_t *vm_object_create_shared(size_t size, uint32_t flags) {
    return vm_object_create_anon(size, flags);
}

vm_object_t *vm_object_create_file(void *backing, uint64_t file_offset, size_t size, uint32_t flags) {
    (void)backing; (void)file_offset; (void)size; (void)flags;
    return NULL;
}

vm_object_t *vm_object_create_device(phys_addr_t phys_base, size_t size, uint32_t cache_flags, uint32_t flags) {
    (void)phys_base; (void)size; (void)cache_flags; (void)flags;
    return NULL;
}

vm_object_t *vm_object_create_dma(phys_addr_t phys_base, size_t size, uint32_t dma_flags, uint32_t numa_node, uint32_t flags) {
    (void)phys_base; (void)size; (void)dma_flags; (void)numa_node; (void)flags;
    return NULL;
}

void vm_object_retain(vm_object_t *obj) {
    if (obj) {
        __atomic_add_fetch(&obj->refcount, 1, __ATOMIC_SEQ_CST);
    }
}

void vm_object_release(vm_object_t *obj) {
    if (obj) {
        if (__atomic_sub_fetch(&obj->refcount, 1, __ATOMIC_SEQ_CST) == 0) {
            if (obj->ops && obj->ops->release) {
                obj->ops->release(obj);
            }
        }
    }
}
