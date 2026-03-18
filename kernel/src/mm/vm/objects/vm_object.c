#include "../../../../include/mm/vm_object.h"
#include "../../../../include/mm/aspace.h"
#include "../../../../include/mm.h"
#include "../../../../include/numa.h"
#include "../../../../include/mm/numa_policy.h"
#include "../../../../include/console.h"
#include "../../../../include/slab.h"

// ============================================================================
// Internal Helpers
// ============================================================================

static vm_object_t *vm_object_alloc(vm_object_kind_t kind, size_t size, uint32_t flags) {
    vm_object_t *obj = (vm_object_t *)kmalloc(sizeof(vm_object_t));
    if (!obj) return NULL;

    obj->kind = kind;
    obj->size = size;
    obj->flags = flags;
    obj->refcount = 1;
    obj->ops = NULL;

    return obj;
}

void vm_object_retain(vm_object_t *obj) {
    if (!obj) return;
    __atomic_fetch_add(&obj->refcount, 1, __ATOMIC_SEQ_CST);
}

void vm_object_release(vm_object_t *obj) {
    if (!obj) return;
    uint32_t ref = __atomic_fetch_sub(&obj->refcount, 1, __ATOMIC_SEQ_CST);
    if (ref == 1) { // Was 1 before sub, now 0
        if (obj->ops && obj->ops->release) {
            obj->ops->release(obj);
        }
        kfree(obj);
    }
}

// ============================================================================
// Generic Stubs
// ============================================================================

static int vm_object_fault_stub(struct vm_object *obj, struct vm_region *region, uintptr_t fault_addr, uint32_t access, phys_addr_t *out_page, uint32_t *out_page_flags) {
    (void)obj;
    (void)region;
    (void)fault_addr;
    (void)access;
    (void)out_page;
    (void)out_page_flags;
    return VM_FAULT_ENOSYS;
}

static void vm_object_release_default(struct vm_object *obj) {
    (void)obj;
}

// ============================================================================
// Anonymous Memory Object
// ============================================================================

static const vm_object_ops_t anon_ops = {
    .fault = vm_object_fault_stub,
    .release = vm_object_release_default
};

vm_object_t *vm_object_create_anon(size_t size, uint32_t flags) {
    vm_object_t *obj = vm_object_alloc(VM_OBJECT_ANON, size, flags);
    if (obj) {
        obj->ops = &anon_ops;
        obj->u.anon.zero_fill = 1;
    }
    return obj;
}

// ============================================================================
// Shared Anonymous Memory Object
// ============================================================================

static const vm_object_ops_t shared_ops = {
    .fault = vm_object_fault_stub,
    .release = vm_object_release_default
};

vm_object_t *vm_object_create_shared(size_t size, uint32_t flags) {
    vm_object_t *obj = vm_object_alloc(VM_OBJECT_SHARED, size, flags);
    if (obj) {
        obj->ops = &shared_ops;
        obj->u.shared.shared_id = 0; // Placeholder
    }
    return obj;
}

// ============================================================================
// File Memory Object
// ============================================================================

static const vm_object_ops_t file_ops = {
    .fault = vm_object_fault_stub,
    .release = vm_object_release_default
};

vm_object_t *vm_object_create_file(void *backing, uint64_t file_offset, size_t size, uint32_t flags) {
    vm_object_t *obj = vm_object_alloc(VM_OBJECT_FILE, size, flags);
    if (obj) {
        obj->ops = &file_ops;
        obj->u.file.backing = backing;
        obj->u.file.file_offset = file_offset;
    }
    return obj;
}

// ============================================================================
// Device Memory Object
// ============================================================================

static const vm_object_ops_t device_ops = {
    .fault = vm_object_fault_stub,
    .release = vm_object_release_default
};

vm_object_t *vm_object_create_device(phys_addr_t phys_base, size_t size, uint32_t cache_flags, uint32_t flags) {
    vm_object_t *obj = vm_object_alloc(VM_OBJECT_DEVICE, size, flags);
    if (obj) {
        obj->ops = &device_ops;
        obj->u.device.phys_base = phys_base;
        obj->u.device.cache_flags = cache_flags;
    }
    return obj;
}

// ============================================================================
// DMA Memory Object
// ============================================================================

static const vm_object_ops_t dma_ops = {
    .fault = vm_object_fault_stub,
    .release = vm_object_release_default
};

vm_object_t *vm_object_create_dma(phys_addr_t phys_base, size_t size, uint32_t dma_flags, uint32_t numa_node, uint32_t flags) {
    vm_object_t *obj = vm_object_alloc(VM_OBJECT_DMA, size, flags);
    if (obj) {
        obj->ops = &dma_ops;
        obj->u.dma.phys_base = phys_base;
        obj->u.dma.dma_flags = dma_flags;
        obj->u.dma.numa_node = numa_node;
    }
    return obj;
}
