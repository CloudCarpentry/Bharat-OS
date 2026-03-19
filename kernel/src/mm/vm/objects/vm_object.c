#include "../../../../include/mm/vm_object.h"
#include "../../../../include/mm/pmm.h"
#include "../../../../include/slab.h"
#include "../../../../include/hal/hal_pt.h"
#include "../../../../include/advanced/formal_verif.h"
#include "../../../../include/numa.h"

static int anon_fault(struct vm_object *obj,
                      struct vm_region *region,
                      uintptr_t fault_addr,
                      uint32_t access,
                      phys_addr_t *out_page,
                      uint32_t *out_page_flags) {
    (void)obj;
    (void)region;
    (void)fault_addr;

    if (!out_page || !out_page_flags) {
        return VM_FAULT_SIGSEGV;
    }

    phys_addr_t page = mm_alloc_page(NUMA_NODE_ANY);
    if (!page) {
        return VM_FAULT_OOM;
    }

    uint32_t pt_flags = HAL_PT_FLAG_READ | HAL_PT_FLAG_USER;
    if (access & CAP_RIGHT_WRITE) {
        pt_flags |= HAL_PT_FLAG_WRITE;
    }
    if (access & CAP_RIGHT_EXECUTE) {
        pt_flags |= HAL_PT_FLAG_EXEC;
    }
    return obj;
}

    *out_page = page;
    *out_page_flags = pt_flags;
    return VM_FAULT_HANDLED;
}

static void vm_object_default_release(struct vm_object *obj) {
    // Release cache or resident pages.
    // Page tables no longer point to this object directly; they are purely a hardware derived cache
    // derived from address_space_t -> region -> vm_object_t. The page frames allocated here can be
    // securely released because the hardware mappings have already been torn down by aspace_destroy/unmap.
    kfree(obj);
}

static const vm_object_ops_t anon_ops = {
    // Note: In phase 1 cleanup, fault resolution strictly consults address_space_t -> region -> vm_object_t
    // and returns the physical page frame up to the VMM layer which installs the derived hardware cache (page table).
    .fault = anon_fault,
    .release = vm_object_default_release,
};

vm_object_t *vm_object_create_anon(size_t size, uint32_t flags) {
    vm_object_t *obj = (vm_object_t *)kmalloc(sizeof(vm_object_t));
    if (!obj) {
        return NULL;
    }

    obj->kind = VM_OBJECT_ANON;
    obj->flags = flags;
    obj->size = size;
    obj->refcount = 1;
    obj->ops = &anon_ops;
    obj->u.anon.zero_fill = 1;

    return obj;
}

vm_object_t *vm_object_create_shared(size_t size, uint32_t flags) {
    vm_object_t *obj = vm_object_create_anon(size, flags);
    if (obj) {
        obj->kind = VM_OBJECT_SHARED;
        obj->u.shared.shared_id = 0;
    }
    return obj;
}

vm_object_t *vm_object_create_file(void *backing, uint64_t file_offset, size_t size, uint32_t flags) {
    vm_object_t *obj = (vm_object_t *)kmalloc(sizeof(vm_object_t));
    if (!obj) {
        return NULL;
    }

    obj->kind = VM_OBJECT_FILE;
    obj->flags = flags;
    obj->size = size;
    obj->refcount = 1;
    obj->ops = &anon_ops;
    obj->u.file.backing = backing;
    obj->u.file.file_offset = file_offset;

    return obj;
}

vm_object_t *vm_object_create_device(phys_addr_t phys_base, size_t size, uint32_t cache_flags, uint32_t flags) {
    vm_object_t *obj = (vm_object_t *)kmalloc(sizeof(vm_object_t));
    if (!obj) {
        return NULL;
    }

    obj->kind = VM_OBJECT_DEVICE;
    obj->flags = flags;
    obj->size = size;
    obj->refcount = 1;
    obj->ops = &anon_ops;
    obj->u.device.phys_base = phys_base;
    obj->u.device.cache_flags = cache_flags;

    return obj;
}

vm_object_t *vm_object_create_dma(phys_addr_t phys_base, size_t size, uint32_t dma_flags, uint32_t numa_node, uint32_t flags) {
    vm_object_t *obj = (vm_object_t *)kmalloc(sizeof(vm_object_t));
    if (!obj) {
        return NULL;
    }

    obj->kind = VM_OBJECT_DMA;
    obj->flags = flags;
    obj->size = size;
    obj->refcount = 1;
    obj->ops = &anon_ops;
    obj->u.dma.phys_base = phys_base;
    obj->u.dma.dma_flags = dma_flags;
    obj->u.dma.numa_node = numa_node;

    return obj;
}
