#include "../../include/mm/vm_space.h"
#include "../../include/mm/vm_mapping.h"
#include "../../include/mm/arch_vm.h"
#include "../../include/monitor/mon_vm_ops.h"
#include "../../include/hal/hal.h"
#include "../../include/mm.h"
#include <stddef.h>

int vm_map(vm_space_t *space, const vm_map_req_t *req) {
    if (!space || !req) return -1;

    spinlock_acquire(&space->lock);

    // Profile check
    if (space->timing_class >= VM_TIMING_FIRM_RT && (req->map_flags & VM_MAP_NO_LAZY_SYNC) == 0) {
        // RT constraints require strict sync
        // return -1; // Uncomment for strict enforcement
    }

    vm_mapping_t *mapping = (vm_mapping_t *)kmalloc(sizeof(vm_mapping_t));
    if (!mapping) {
        spinlock_release(&space->lock);
        return -1;
    }

    mapping->va_start = req->va;
    mapping->pa_start = req->pa;
    mapping->length = req->len;
    mapping->prot = req->prot;
    mapping->mem_type = req->mem_type;
    mapping->flags = req->map_flags;
    mapping->zone = req->zone;
    mapping->map_gen = ++space->generation;
    mapping->next = space->mappings.head;
    mapping->prev = NULL;

    if (space->mappings.head) {
        space->mappings.head->prev = mapping;
    }
    space->mappings.head = mapping;

    bool is_strict = (req->map_flags & VM_MAP_NO_LAZY_SYNC) || (space->timing_class >= VM_TIMING_FIRM_RT);

    // Coordinate with Monitor Plane
    mon_vm_send_map(space, req, is_strict);

    spinlock_release(&space->lock);

    // If local core is active, realize immediately
    if (space->active_cores & (1ULL << hal_get_core_id())) {
        if (active_arch_vm_ops && active_arch_vm_ops->map) {
            vm_core_state_t local_state = { .core_id = hal_get_core_id() };
            active_arch_vm_ops->map(space, &local_state, req->va, req->pa, req->len, req->prot, req->mem_type, req->map_flags);
        }
    }

    return 0;
}

int vm_unmap(vm_space_t *space, uintptr_t va, size_t len) {
    if (!space) return -1;

    spinlock_acquire(&space->lock);

    vm_mapping_t *curr = space->mappings.head;
    while (curr) {
        if (curr->va_start == va && curr->length == len) {
            if (curr->prev) curr->prev->next = curr->next;
            else space->mappings.head = curr->next;

            if (curr->next) curr->next->prev = curr->prev;

            curr->map_gen = ++space->generation; // Bump generation for revoke

            // Coordinate with Monitor Plane (Unmap MUST be strict)
            mon_vm_send_unmap(space, va, len, true);

            kfree(curr);
            break;
        }
        curr = curr->next;
    }

    spinlock_release(&space->lock);

    // Local invalidation
    if (space->active_cores & (1ULL << hal_get_core_id())) {
        if (active_arch_vm_ops && active_arch_vm_ops->unmap) {
            vm_core_state_t local_state = { .core_id = hal_get_core_id() };
            active_arch_vm_ops->unmap(space, &local_state, va, len);
        }
    }

    return 0;
}

int vm_protect(vm_space_t *space, uintptr_t va, size_t len, uint64_t prot, uint64_t mem_type) {
    if (!space) return -1;

    spinlock_acquire(&space->lock);

    vm_mapping_t *curr = space->mappings.head;
    while (curr) {
        if (curr->va_start == va && curr->length == len) {
            curr->prot = prot;
            curr->mem_type = mem_type;
            curr->map_gen = ++space->generation;
            break;
        }
        curr = curr->next;
    }

    // Downgrades should be strict, upgrades can be lazy. Assuming strict for now.
    mon_vm_send_protect(space, va, len, prot, mem_type, true);

    spinlock_release(&space->lock);

    if (space->active_cores & (1ULL << hal_get_core_id())) {
        if (active_arch_vm_ops && active_arch_vm_ops->protect) {
            vm_core_state_t local_state = { .core_id = hal_get_core_id() };
            active_arch_vm_ops->protect(space, &local_state, va, len, prot, mem_type);
        }
    }

    return 0;
}
