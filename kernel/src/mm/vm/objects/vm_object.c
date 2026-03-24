#include "../../../../include/mm/vm_object.h"
#include "../../../../include/mm/pmm.h"
#include "../../../../include/mm/aspace.h"
#include "../../../../include/hal/hal_pt.h"
#include "../../../../include/hal/hal_tlb.h"
#include "../../../../include/slab.h"

#define PAGE_ALIGNED(x) (((x) & (PAGE_SIZE - 1)) == 0)

static vm_object_t *vm_object_alloc_common(vm_object_kind_t kind, size_t size, uint32_t flags) {
    vm_object_t *obj = (vm_object_t *)kmalloc(sizeof(vm_object_t));
    if (!obj) {
        return NULL;
    }

    __builtin_memset(obj, 0, sizeof(vm_object_t));
    obj->kind = kind;
    obj->size = size;
    obj->flags = flags;
    obj->magic = 0x564D4F42; // "VMOB"
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

// VM Object Backend - Shared Memory

static int shared_fault(struct vm_object *obj, struct vm_region *region, uintptr_t fault_addr, uint32_t access, phys_addr_t *out_page, uint32_t *out_page_flags) {
    return anon_fault(obj, region, fault_addr, access, out_page, out_page_flags);
}

static void shared_destroy(struct vm_object *obj) {
    (void)obj;
}

static vm_object_ops_t shared_ops = {
    .fault = shared_fault,
    .release = shared_destroy,
};

// VM Object Backend - File Backed

static int file_fault(struct vm_object *obj, struct vm_region *region, uintptr_t fault_addr, uint32_t access, phys_addr_t *out_page, uint32_t *out_page_flags) {
    if (!obj) return -1;
    (void)region;
    (void)fault_addr;
    (void)access;
    (void)out_page;
    (void)out_page_flags;
    return VM_FAULT_ENOSYS;
}

static void file_destroy(struct vm_object *obj) {
    (void)obj;
}

static vm_object_ops_t file_ops = {
    .fault = file_fault,
    .release = file_destroy,
};

// VM Object Backend - Device

static int device_fault(struct vm_object *obj, struct vm_region *region, uintptr_t fault_addr, uint32_t access, phys_addr_t *out_page, uint32_t *out_page_flags) {
    if (!obj) return -1;
    (void)region;
    (void)access;

    if (out_page) {
        uintptr_t offset = fault_addr - region->base;
        *out_page = obj->u.device.phys_base + offset;
    }
    if (out_page_flags) *out_page_flags = access; // Cache flags could be appended
    return VM_FAULT_HANDLED;
}

static void device_destroy(struct vm_object *obj) {
    (void)obj;
}

static vm_object_ops_t device_ops = {
    .fault = device_fault,
    .release = device_destroy,
};

// VM Object Backend - DMA

static int dma_fault(struct vm_object *obj, struct vm_region *region, uintptr_t fault_addr, uint32_t access, phys_addr_t *out_page, uint32_t *out_page_flags) {
    if (!obj) return -1;
    (void)region;
    (void)access;

    if (out_page) {
        uintptr_t offset = fault_addr - region->base;
        *out_page = obj->u.dma.phys_base + offset;
    }
    if (out_page_flags) *out_page_flags = access;
    return VM_FAULT_HANDLED;
}

static void dma_destroy(struct vm_object *obj) {
    (void)obj;
}

static vm_object_ops_t dma_ops = {
    .fault = dma_fault,
    .release = dma_destroy,
};


vm_object_t *vm_object_create_anon(size_t size, uint32_t flags) {
    if (size == 0 || !PAGE_ALIGNED(size)) {
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
    if (size == 0 || !PAGE_ALIGNED(size)) {
        return NULL;
    }

    vm_object_t *obj = vm_object_alloc_common(VM_OBJECT_SHARED, size, flags);
    if (!obj) {
        return NULL;
    }

    obj->ops = &shared_ops;
    obj->u.shared.shared_id = 0;
    return obj;
}

vm_object_t *vm_object_create_file(void *backing, uint64_t file_offset, size_t size, uint32_t flags) {
    if (!backing || size == 0 || !PAGE_ALIGNED(size)) {
        return NULL;
    }
    if (file_offset + size < file_offset) { // Overflow check
        return NULL;
    }

    vm_object_t *obj = vm_object_alloc_common(VM_OBJECT_FILE, size, flags);
    if (!obj) {
        return NULL;
    }

    obj->ops = &file_ops;
    obj->u.file.backing = backing;
    obj->u.file.file_offset = file_offset;
    return obj;
}

vm_object_t *vm_object_create_device(phys_addr_t phys_base, size_t size, uint32_t cache_flags, uint32_t flags) {
    if (phys_base == 0 || size == 0 || !PAGE_ALIGNED(size) || !PAGE_ALIGNED(phys_base)) {
        return NULL;
    }
    if (phys_base + size < phys_base) { // Overflow check
        return NULL;
    }

    vm_object_t *obj = vm_object_alloc_common(VM_OBJECT_DEVICE, size, flags);
    if (!obj) {
        return NULL;
    }

    obj->ops = &device_ops;
    obj->u.device.phys_base = phys_base;
    obj->u.device.cache_flags = cache_flags;
    return obj;
}

vm_object_t *vm_object_create_dma(phys_addr_t phys_base, size_t size, uint32_t dma_flags, uint32_t numa_node, uint32_t flags) {
    if (phys_base == 0 || size == 0 || !PAGE_ALIGNED(size) || !PAGE_ALIGNED(phys_base)) {
        return NULL;
    }
    if (phys_base + size < phys_base) { // Overflow check
        return NULL;
    }

    vm_object_t *obj = vm_object_alloc_common(VM_OBJECT_DMA, size, flags);
    if (!obj) {
        return NULL;
    }

    obj->ops = &dma_ops;
    obj->u.dma.phys_base = phys_base;
    obj->u.dma.dma_flags = dma_flags;
    obj->u.dma.numa_node = numa_node;
    return obj;
}
