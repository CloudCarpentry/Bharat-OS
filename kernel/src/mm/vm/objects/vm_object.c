#include "../../include/mm/vm_object.h"
#include "../../include/mm/pmm.h"
#include "../../include/mm/aspace.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/hal/hal_tlb.h"

// VM Object Backend - Anonymous Memory

static void anon_release(vm_object_t *obj) {
    // Release cache or resident pages.
    // Page tables no longer point to this object directly; they are purely a hardware derived cache
    // derived from address_space_t -> region -> vm_object_t. The page frames allocated here can be
    // securely released because the hardware mappings have already been torn down by aspace_destroy/unmap.
    if (obj) {
        // Free object's resident pages metadata (simplified for stub)
    }
}

static const vm_object_ops_t anon_ops = {
    // Note: In phase 1 cleanup, fault resolution strictly consults address_space_t -> region -> vm_object_t
    // and returns the physical page frame up to the VMM layer which installs the derived hardware cache (page table).
    // .fault = anon_fault,
    .release = anon_release,
};

vm_object_t *vm_object_create_anon_size(uint64_t size) {
    // Allocate object metadata (for now simplified static allocation or kmem)
    static vm_object_t static_anon_pool[128];
    static int pool_idx = 0;

    vm_object_t *obj = &static_anon_pool[pool_idx++];
    obj->size = size;
    obj->refcount = 1;
    obj->kind = VM_OBJECT_ANON;
    obj->ops = &anon_ops;
    return obj;
}

vm_object_t *vm_object_create_anon(size_t size, uint32_t flags) {
    return vm_object_create_anon_size(size);
}

vm_object_t *vm_object_create_shared(size_t size, uint32_t flags) {
    return vm_object_create_anon(size, flags); // Placeholder
}

vm_object_t *vm_object_create_file(void *backing, uint64_t file_offset, size_t size, uint32_t flags) {
    (void)backing; (void)file_offset; (void)size; (void)flags;
    return NULL; // Stubbed for now
}
