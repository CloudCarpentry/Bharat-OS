#include "../../include/mm/aspace.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/hal/hal_tlb.h"
#include "../../include/mm.h"
#include "../../include/spinlock.h"
#include "../../include/numa.h"

// Basic slab allocator or kmalloc substitute for region nodes
#include "../../include/slab.h"

static uint64_t next_as_id = 1;

int aspace_create(vm_address_space_t **out_aspace) {
    if (!out_aspace) return -1;

    vm_address_space_t *aspace = (vm_address_space_t *)kmalloc(sizeof(vm_address_space_t));
    if (!aspace) return -2;

    if (!active_hal_pt) hal_pt_init();

    extern phys_addr_t vmm_get_kernel_root(void);
    aspace->root_pt = active_hal_pt->create_address_space(vmm_get_kernel_root());
    if (aspace->root_pt == 0) {
        kfree(aspace);
        return -3;
    }

    aspace->object_id = __atomic_fetch_add(&next_as_id, 1, __ATOMIC_SEQ_CST);
    spin_lock_init(&aspace->lock);
    aspace->regions = NULL;

    // Generic user ranges for 64-bit systems
    aspace->user_base = 0x1000;
    aspace->user_limit = 0x00007FFFFFFFFFFF;

    *out_aspace = aspace;
    return 0;
}

int aspace_destroy(vm_address_space_t *aspace) {
    if (!aspace) return -1;

    spin_lock(&aspace->lock);

    // Walk and free regions
    vm_region_t *curr = aspace->regions;
    while (curr) {
        vm_region_t *next = curr->next;

        // TODO: Call object->ops->unmap(object, curr->object_offset, curr->length)

        // Unmap from hardware page tables
        for (uint64_t va = curr->base; va < curr->base + curr->length; va += PAGE_SIZE) {
            phys_addr_t paddr = 0;
            if (active_hal_pt->unmap_page(aspace->root_pt, va, &paddr) == 0) {
                if (paddr != 0) {
                    mm_free_page(paddr); // Assuming anonymous memory for now, proper logic needed per object type
                }
            }
        }

        kfree(curr);
        curr = next;
    }
    aspace->regions = NULL;

    if (active_hal_pt) {
        active_hal_pt->destroy_address_space(aspace->root_pt);
    }

    spin_unlock(&aspace->lock);
    kfree(aspace);
    return 0;
}

bool aspace_check_overlap(vm_address_space_t *aspace, uint64_t base, uint64_t length) {
    if (!aspace) return false;

    uint64_t end = base + length;

    vm_region_t *curr = aspace->regions;
    while (curr) {
        uint64_t curr_end = curr->base + curr->length;

        // Overlap condition
        if (base < curr_end && end > curr->base) {
            return true;
        }

        curr = curr->next;
    }

    return false;
}

int aspace_find_region(vm_address_space_t *aspace, uint64_t vaddr, vm_region_t **out_region) {
    if (!aspace || !out_region) return -1;

    spin_lock(&aspace->lock);

    vm_region_t *curr = aspace->regions;
    while (curr) {
        if (vaddr >= curr->base && vaddr < curr->base + curr->length) {
            *out_region = curr;
            spin_unlock(&aspace->lock);
            return 0;
        }
        curr = curr->next;
    }

    spin_unlock(&aspace->lock);
    return -2; // Not found
}

static int insert_region_sorted(vm_address_space_t *aspace, vm_region_t *new_region) {
    if (!aspace->regions) {
        aspace->regions = new_region;
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

    return 0;
}

static uint64_t find_free_hole(vm_address_space_t *aspace, uint64_t length) {
    uint64_t current_base = aspace->user_base;

    vm_region_t *curr = aspace->regions;
    while (curr) {
        if (curr->base - current_base >= length) {
            return current_base; // Found a hole big enough
        }
        current_base = curr->base + curr->length;
        curr = curr->next;
    }

    if (aspace->user_limit - current_base >= length) {
        return current_base;
    }

    return 0; // Out of memory
}

int aspace_map_region(vm_address_space_t *aspace, uint64_t vaddr_hint, uint64_t length, uint32_t prot, uint32_t map_flags, vm_object_t *object, uint64_t object_offset, uint64_t *out_vaddr) {
    if (!aspace || length == 0) return -1;

    length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    spin_lock(&aspace->lock);

    uint64_t base = vaddr_hint;
    if (base != 0) {
        base = base & ~(PAGE_SIZE - 1);
        if (aspace_check_overlap(aspace, base, length) || base < aspace->user_base || base + length > aspace->user_limit) {
            spin_unlock(&aspace->lock);
            return -2; // Hint invalid
        }
    } else {
        base = find_free_hole(aspace, length);
        if (base == 0) {
            spin_unlock(&aspace->lock);
            return -3; // No free space
        }
    }

    vm_region_t *region = (vm_region_t *)kmalloc(sizeof(vm_region_t));
    if (!region) {
        spin_unlock(&aspace->lock);
        return -4;
    }

    region->base = base;
    region->length = length;
    region->prot = prot;
    region->map_flags = map_flags;
    region->object = object;
    region->object_offset = object_offset;
    region->next = NULL;
    region->prev = NULL;

    insert_region_sorted(aspace, region);

    // Normally, demand paging would fault these in. For MAP_POPULATE or equivalent, we map now.
    // If no object, assume anonymous memory and map zeros or leave it for fault handler.

    spin_unlock(&aspace->lock);

    if (out_vaddr) *out_vaddr = base;
    return 0;
}

int aspace_unmap_region(vm_address_space_t *aspace, uint64_t base, uint64_t length) {
    if (!aspace || length == 0) return -1;

    base = base & ~(PAGE_SIZE - 1);
    length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    spin_lock(&aspace->lock);

    vm_region_t *curr = aspace->regions;
    while (curr) {
        if (curr->base >= base && curr->base + curr->length <= base + length) {
            // Full overlap, remove region
            if (curr->prev) curr->prev->next = curr->next;
            else aspace->regions = curr->next;
            if (curr->next) curr->next->prev = curr->prev;

            // Unmap from page tables
            for (uint64_t va = curr->base; va < curr->base + curr->length; va += PAGE_SIZE) {
                phys_addr_t paddr = 0;
                if (active_hal_pt->unmap_page(aspace->root_pt, va, &paddr) == 0) {
                    if (paddr != 0) mm_free_page(paddr);
                }
            }

            vm_region_t *next = curr->next;
            kfree(curr);
            curr = next;
        } else if (curr->base < base + length && curr->base + curr->length > base) {
            // Partial overlap, split or truncate (complex, leaving simple stub for now)
            curr = curr->next;
        } else {
            curr = curr->next;
        }
    }

    spin_unlock(&aspace->lock);
    return 0;
}

int aspace_protect_region(vm_address_space_t *aspace, uint64_t base, uint64_t length, uint32_t new_prot) {
    if (!aspace || length == 0) return -1;

    base = base & ~(PAGE_SIZE - 1);
    length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    spin_lock(&aspace->lock);

    vm_region_t *curr = aspace->regions;
    while (curr) {
        if (curr->base >= base && curr->base + curr->length <= base + length) {
            curr->prot = new_prot;

            // Update page tables
            for (uint64_t va = curr->base; va < curr->base + curr->length; va += PAGE_SIZE) {
                // Determine PT flags from prot
                uint32_t pt_flags = HAL_PT_FLAG_READ | HAL_PT_FLAG_USER;
                if (new_prot & 2) pt_flags |= HAL_PT_FLAG_WRITE; // Assume PROT_WRITE=2
                if (new_prot & 4) pt_flags |= HAL_PT_FLAG_EXEC;  // Assume PROT_EXEC=4

                active_hal_pt->protect_page(aspace->root_pt, va, pt_flags);
            }
        }
        curr = curr->next;
    }

    spin_unlock(&aspace->lock);
    return 0;
}
