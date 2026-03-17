#include "../../include/mm/vm_object.h"
#include "../../include/mm.h"
#include "../../include/numa.h"
#include "../../include/mm/numa_policy.h"
#include "../../include/console.h"

// Basic slab allocator or kmalloc substitute for region nodes
#include "../../include/slab.h"

int vm_object_create(vm_object_kind_t kind, uint64_t size, vm_object_t **out_obj) {
    if (!out_obj || size == 0) return -1;

    vm_object_t *obj = (vm_object_t *)kmalloc(sizeof(vm_object_t));
    if (!obj) return -2;

    obj->kind = kind;
    obj->size_bytes = size;
    obj->object_flags = 0;
    obj->refcount = 1;
    spin_lock_init(&obj->lock);
    obj->ops = NULL;
    obj->backend_data = NULL;

    *out_obj = obj;
    return 0;
}

int vm_object_ref(vm_object_t *obj) {
    if (!obj) return -1;
    __atomic_fetch_add(&obj->refcount, 1, __ATOMIC_SEQ_CST);
    return 0;
}

int vm_object_unref(vm_object_t *obj) {
    if (!obj) return -1;
    uint32_t ref = __atomic_fetch_sub(&obj->refcount, 1, __ATOMIC_SEQ_CST);
    if (ref == 1) { // Was 1 before sub, now 0
        if (obj->ops && obj->ops->release) {
            obj->ops->release(obj);
        }
        kfree(obj);
    }
    return 0;
}

// ============================================================================
// Anonymous Memory Object (Zero-Fill-On-Demand)
// ============================================================================

static int anon_fault(vm_object_t *obj, const vm_fault_ctx_t *ctx, uint64_t *out_phys_page) {
    if (!obj || !ctx || !out_phys_page) return VM_FAULT_SIGSEGV;

    // Allocate a new physical page with NUMA awareness
    numa_affinity_t local_policy = { .policy = NUMA_POLICY_LOCAL_PREFERRED, .target_node = NUMA_NODE_ANY };
    phys_addr_t paddr = mm_alloc_page_policy(&local_policy);
    if (paddr == 0) return VM_FAULT_OOM;

    // Zero-Fill-On-Demand
    // Assuming P2V maps all physical memory into a direct map region
    #define P2V(x) ((void*)(uintptr_t)(x))
    uint8_t *page_ptr = (uint8_t *)P2V(paddr);
    for (int i = 0; i < PAGE_SIZE; i++) {
        page_ptr[i] = 0;
    }

    *out_phys_page = paddr;
    return VM_FAULT_HANDLED;
}

static void anon_release(vm_object_t *obj) {
    // In a production kernel, the vm_object tracks the physical pages via a radix tree or list
    // so they can be reclaimed or freed properly when the object dies or is truncated.
    (void)obj;
}

static const vm_object_ops_t anon_ops = {
    .fault = anon_fault,
    .writeback = NULL,
    .map_notify = NULL,
    .unmap_notify = NULL,
    .release = anon_release
};

int vm_object_create_anon(uint64_t size, vm_object_t **out_obj) {
    int ret = vm_object_create(VM_OBJECT_ANON, size, out_obj);
    if (ret == 0) {
        (*out_obj)->ops = &anon_ops;
    }
    return ret;
}

// ============================================================================
// Shared Anonymous Memory Object (Shared Memory / COW Base)
// ============================================================================

static int shared_fault(vm_object_t *obj, const vm_fault_ctx_t *ctx, uint64_t *out_phys_page) {
    if (!obj || !ctx || !out_phys_page) return VM_FAULT_SIGSEGV;

    spin_lock(&obj->lock);

    numa_affinity_t local_policy = { .policy = NUMA_POLICY_LOCAL_PREFERRED, .target_node = NUMA_NODE_ANY };
    phys_addr_t paddr = mm_alloc_page_policy(&local_policy);
    if (paddr == 0) {
        spin_unlock(&obj->lock);
        return VM_FAULT_OOM;
    }

    uint8_t *page_ptr = (uint8_t *)P2V(paddr);
    for (int i = 0; i < PAGE_SIZE; i++) {
        page_ptr[i] = 0;
    }

    *out_phys_page = paddr;

    spin_unlock(&obj->lock);
    return VM_FAULT_HANDLED;
}

static void shared_release(vm_object_t *obj) {
    (void)obj;
}

static const vm_object_ops_t shared_ops = {
    .fault = shared_fault,
    .writeback = NULL,
    .map_notify = NULL,
    .unmap_notify = NULL,
    .release = shared_release
};

int vm_object_create_shared(uint64_t size, vm_object_t **out_obj) {
    int ret = vm_object_create(VM_OBJECT_SHARED_ANON, size, out_obj);
    if (ret == 0) {
        (*out_obj)->ops = &shared_ops;
    }
    return ret;
}

// ============================================================================
// Device/DMA Memory Objects (MMIO Stubs)
// ============================================================================

int vm_object_create_device(uint64_t phys_base, uint64_t size, vm_object_t **out_obj) {
    int ret = vm_object_create(VM_OBJECT_DEVICE, size, out_obj);
    if (ret == 0) {
        (*out_obj)->backend_data = (void*)(uintptr_t)phys_base;
    }
    return ret;
}

int vm_object_create_dma(uint64_t size, vm_object_t **out_obj) {
    int ret = vm_object_create(VM_OBJECT_DMA, size, out_obj);
    return ret;
}
