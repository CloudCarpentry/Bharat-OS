#include "../../../../include/mm/vm_object.h"
#include "../../../../include/mm/pmm.h"
#include "../../../../include/mm/aspace.h"
#include "../../../../include/hal/hal_pt.h"
#include "../../../../include/hal/hal_tlb.h"
#include "../../../../include/slab.h"

static vm_object_t *vm_object_alloc_common(vm_object_kind_t kind, size_t size, uint32_t flags) {
    vm_object_t *obj = (vm_object_t *)kmalloc(sizeof(vm_object_t));
    if (!obj) {
        return NULL;
    }

    __builtin_memset(obj, 0, sizeof(vm_object_t));
    obj->kind = kind;
    obj->size = size;
    obj->flags = flags;
    obj->refcount = 1;
    return obj;
}

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
    (void)obj;
}

static vm_object_ops_t anon_ops = {
    .fault = anon_fault,
    .release = anon_destroy,
};

vm_object_t *vm_object_create_anon(size_t size, uint32_t flags) {
    if (size == 0) {
        return NULL;
    }

    vm_object_t *obj = vm_object_alloc_common(VM_OBJECT_ANON, size, flags);
    if (!obj) {
        return NULL;
    }

    obj->ops = &anon_ops;
    obj->u.anon.zero_fill = 1;

    return obj;
}

vm_object_t *vm_object_create_shared(size_t size, uint32_t flags) {
    vm_object_t *obj = vm_object_create_anon(size, flags);
    if (!obj) {
        return NULL;
    }
    obj->kind = VM_OBJECT_SHARED;
    obj->u.shared.shared_id = 0;
    return obj;
}

vm_object_t *vm_object_create_file(void *backing, uint64_t file_offset, size_t size, uint32_t flags) {
    if (!backing || size == 0) {
        return NULL;
    }

    vm_object_t *obj = vm_object_alloc_common(VM_OBJECT_FILE, size, flags);
    if (!obj) {
        return NULL;
    }

    obj->u.file.backing = backing;
    obj->u.file.file_offset = file_offset;
    return obj;
}

vm_object_t *vm_object_create_device(phys_addr_t phys_base, size_t size, uint32_t cache_flags, uint32_t flags) {
    if (phys_base == 0 || size == 0) {
        return NULL;
    }

    vm_object_t *obj = vm_object_alloc_common(VM_OBJECT_DEVICE, size, flags);
    if (!obj) {
        return NULL;
    }

    obj->u.device.phys_base = phys_base;
    obj->u.device.cache_flags = cache_flags;
    return obj;
}

vm_object_t *vm_object_create_dma(phys_addr_t phys_base, size_t size, uint32_t dma_flags, uint32_t numa_node, uint32_t flags) {
    if (phys_base == 0 || size == 0) {
        return NULL;
    }

    vm_object_t *obj = vm_object_alloc_common(VM_OBJECT_DMA, size, flags);
    if (!obj) {
        return NULL;
    }

    obj->u.dma.phys_base = phys_base;
    obj->u.dma.dma_flags = dma_flags;
    obj->u.dma.numa_node = numa_node;
    return obj;
}
