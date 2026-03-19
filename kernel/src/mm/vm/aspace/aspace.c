#include "../../../../include/mm/aspace.h"
#include "../../../../include/hal/hal_pt.h"
#include "../../../../include/hal/hal_tlb.h"
#include "../../../../include/mm.h"
#include "../../../../include/spinlock.h"
#include "../../../../include/numa.h"
#include "../../../../include/slab.h"

static uint64_t next_as_id = 1;

int aspace_create(address_space_t **out_aspace, uint32_t flags) {
    if (!out_aspace) return -1;

    address_space_t *as = (address_space_t *)kmalloc(sizeof(address_space_t));
    if (!as) return -1;

    if (!active_hal_pt) hal_pt_init();

    extern phys_addr_t vmm_get_kernel_root(void);
    as->root_pt = active_hal_pt->create_address_space(vmm_get_kernel_root());
    if (as->root_pt == 0) {
        kfree(as);
        return -1;
    }

    as->object_id = __atomic_fetch_add(&next_as_id, 1, __ATOMIC_SEQ_CST);
    spin_lock_init(&as->lock);
    as->regions = NULL;
    as->region_count = 0;
    as->flags = flags;
    as->owner = NULL;
    as->timing_class = 0; // default VM_TIMING_BEST_EFFORT

    as->user_base = 0x1000;
    as->user_limit = 0x00007FFFFFFFFFFF;

    *out_aspace = as;
    return 0;
}

int aspace_destroy(address_space_t *aspace) {
    if (!aspace) return -1;

    spin_lock(&aspace->lock);

    vm_region_t *curr = aspace->regions;
    while (curr) {
        vm_region_t *next = curr->next;

        // Note: For now, just drop the reference to the object.
        if (curr->object) {
            vm_object_release(curr->object);
        }

        kfree(curr);
        curr = next;
    }
    aspace->regions = NULL;
    aspace->region_count = 0;

    if (active_hal_pt) {
        active_hal_pt->destroy_address_space(aspace->root_pt);
    }

    spin_unlock(&aspace->lock);
    kfree(aspace);
    return 0;
}

static inline bool range_overlaps(uintptr_t a_base, uintptr_t a_end, uintptr_t b_base, uintptr_t b_end) {
    return !(a_end <= b_base || a_base >= b_end);
}

bool aspace_check_overlap(address_space_t *aspace, uint64_t base, uint64_t length) {
    if (!aspace || length == 0) return true; // Invalid query
    uintptr_t end = base + length;
    if (end <= base) return true; // Overflow

    vm_region_t *curr = aspace->regions;
    while (curr) {
        if (range_overlaps(base, end, curr->base, curr->base + curr->length)) {
            return true;
        }
        curr = curr->next;
    }
    return false;
}

vm_region_t *aspace_lookup_region(address_space_t *aspace, uintptr_t va) {
    if (!aspace) return NULL;

    vm_region_t *curr = aspace->regions;
    while (curr) {
        uintptr_t end = curr->base + curr->length;
        if (va >= curr->base && va < end) {
            return curr;
        }
        if (va < curr->base) {
            // Because the list is sorted, we can break early
            return NULL;
        }
        curr = curr->next;
    }
    return NULL;
}

int aspace_find_region(address_space_t *aspace, uint64_t vaddr, vm_region_t **out_region) {
    vm_region_t *r = aspace_lookup_region(aspace, vaddr);
    if (r) {
        if (out_region) *out_region = r;
        return 0;
    }
    return -1;
}

vm_object_t *aspace_lookup_object(address_space_t *aspace, uintptr_t va, vm_region_t **out_region, uint64_t *out_object_offset) {
    vm_region_t *r = aspace_lookup_region(aspace, va);
    if (!r || !r->object) return NULL;

    if (out_region) *out_region = r;
    if (out_object_offset) {
        *out_object_offset = r->object_offset + (uint64_t)(va - r->base);
    }
    return r->object;
}

static int insert_region_sorted(address_space_t *aspace, vm_region_t *new_region) {
    if (!aspace->regions) {
        aspace->regions = new_region;
        new_region->prev = NULL;
        new_region->next = NULL;
        aspace->region_count++;
        return 0;
    }

    vm_region_t *curr = aspace->regions;
    vm_region_t *prev = NULL;

    while (curr && curr->base < new_region->base) {
        prev = curr;
        curr = curr->next;
    }

    new_region->next = curr;
    new_region->prev = prev;

    if (prev) {
        prev->next = new_region;
    } else {
        aspace->regions = new_region;
    }

    if (curr) {
        curr->prev = new_region;
    }

    aspace->region_count++;
    return 0;
}

int aspace_region_reserve(address_space_t *aspace, uintptr_t base, size_t length, uint32_t prot, uint32_t map_flags, vm_inherit_t inherit, vm_region_t **out_region) {
    if (!aspace || length == 0) return -1;

    base = base & ~(PAGE_SIZE - 1);
    length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (base + length <= base) return -1; // Overflow

    spin_lock(&aspace->lock);

    if (aspace_check_overlap(aspace, base, length)) {
        spin_unlock(&aspace->lock);
        return -1;
    }

    vm_region_t *region = (vm_region_t *)kmalloc(sizeof(vm_region_t));
    if (!region) {
        spin_unlock(&aspace->lock);
        return -1;
    }

    region->base = base;
    region->length = length;
    region->prot = prot;
    region->map_flags = map_flags;
    region->region_flags = 0;
    region->inherit = inherit;
    region->object = NULL;
    region->object_offset = 0;

    insert_region_sorted(aspace, region);

    spin_unlock(&aspace->lock);

    if (out_region) *out_region = region;
    return 0;
}

int aspace_region_attach(address_space_t *aspace, uintptr_t base, size_t length, uint32_t prot, uint32_t map_flags, vm_inherit_t inherit, vm_object_t *object, uint64_t object_offset, vm_region_t **out_region) {
    if (!aspace || length == 0) return -1;

    base = base & ~(PAGE_SIZE - 1);
    length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (base + length <= base) return -1; // Overflow

    spin_lock(&aspace->lock);

    if (aspace_check_overlap(aspace, base, length)) {
        spin_unlock(&aspace->lock);
        return -1;
    }

    vm_region_t *region = (vm_region_t *)kmalloc(sizeof(vm_region_t));
    if (!region) {
        spin_unlock(&aspace->lock);
        return -1;
    }

    if (object) {
        vm_object_retain(object);
    }

    region->base = base;
    region->length = length;
    region->prot = prot;
    region->map_flags = map_flags;
    region->region_flags = 0;
    region->inherit = inherit;
    region->object = object;
    region->object_offset = object_offset;

    insert_region_sorted(aspace, region);

    spin_unlock(&aspace->lock);

    if (out_region) *out_region = region;
    return 0;
}

int aspace_region_detach(address_space_t *aspace, uintptr_t base) {
    if (!aspace) return -1;

    base = base & ~(PAGE_SIZE - 1);

    spin_lock(&aspace->lock);

    vm_region_t *curr = aspace->regions;
    while (curr) {
        if (curr->base == base) {
            if (curr->prev) curr->prev->next = curr->next;
            else aspace->regions = curr->next;
            if (curr->next) curr->next->prev = curr->prev;

            aspace->region_count--;

            if (curr->object) {
                vm_object_release(curr->object);
            }

            kfree(curr);
            spin_unlock(&aspace->lock);
            return 0;
        }
        curr = curr->next;
    }

    spin_unlock(&aspace->lock);
    return -1;
}

int aspace_clone(address_space_t *src, address_space_t **out_clone, uint32_t clone_flags) {
    if (!src || !out_clone) return -1;

    address_space_t *dst = NULL;
    int ret = aspace_create(&dst, clone_flags);
    if (ret != 0) return ret;

    spin_lock(&src->lock);

    vm_region_t *curr = src->regions;
    while (curr) {
        if (curr->inherit != VM_INHERIT_NONE) {
            vm_region_t *new_region = (vm_region_t *)kmalloc(sizeof(vm_region_t));
            if (!new_region) {
                spin_unlock(&src->lock);
                aspace_destroy(dst);
                return -1;
            }

            new_region->base = curr->base;
            new_region->length = curr->length;
            new_region->prot = curr->prot;
            new_region->map_flags = curr->map_flags;
            new_region->region_flags = curr->region_flags;
            new_region->inherit = curr->inherit;
            new_region->object = curr->object;
            new_region->object_offset = curr->object_offset;

            if (new_region->object) {
                vm_object_retain(new_region->object);
            }

            insert_region_sorted(dst, new_region);
        }
        curr = curr->next;
    }

    spin_unlock(&src->lock);

    *out_clone = dst;
    return 0;
}

// ============================================================================
// Legacy placeholders
// ============================================================================

int aspace_map_region(address_space_t *aspace, uint64_t vaddr_hint, uint64_t length, uint32_t prot, uint32_t map_flags, vm_object_t *object, uint64_t object_offset, uint64_t *out_vaddr) {
    // Basic wrapper to map legacy calls to the new attach logic.
    // If vaddr_hint is 0, we'd normally search for a hole. In the new logic, attach requires base.
    // For now, this is just a quick placeholder, and we'll let existing tests fail or pass based on hardcoded hints.
    if (vaddr_hint == 0) {
        return -1; // Not fully supported by legacy adapter
    }

    vm_region_t *r = NULL;
    int ret = aspace_region_attach(aspace, vaddr_hint, length, prot, map_flags, VM_INHERIT_COPY_META, object, object_offset, &r);
    if (ret == 0 && out_vaddr) *out_vaddr = vaddr_hint;
    return ret;
}

int aspace_unmap_region(address_space_t *aspace, uint64_t base, uint64_t length) {
    (void)length; // Assume unmapping the whole region matched by base
    return aspace_region_detach(aspace, base);
}

int aspace_protect_region(address_space_t *aspace, uint64_t base, uint64_t length, uint32_t new_prot) {
    (void)length;
    vm_region_t *r = aspace_lookup_region(aspace, base);
    if (r) {
        r->prot = new_prot;
        return 0;
    }
    return -1;
}
