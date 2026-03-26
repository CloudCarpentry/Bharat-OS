#include "../../../../include/mm/vm_space.h"
#include "../../../../include/mm/vm_mapping.h"
#include "../../../../include/monitor/mon_vm_ops.h"
#include "../../../../include/hal/hal.h"
#include "../../../../include/mm.h"
#include "../../../../include/slab.h"
#include "../../../../include/spinlock.h"
#include "../../../../include/kernel_safety.h"
#include "../../../../include/multicore.h"
#include "../../../../include/mm/arch_vm.h"
#include <stddef.h>

// Mock hal_get_core_id if it's not defined
#ifndef hal_get_core_id
extern uint32_t hal_get_core_id(void);
#endif

// Mock spinlock_acquire/release if they are not defined in spinlock.h
#ifndef spinlock_acquire
#define spinlock_acquire spin_lock
#endif
#ifndef spinlock_release
#define spinlock_release spin_unlock
#endif

int vm_map(vm_space_t *space, const vm_map_req_t *req) {
    if (!space || !req) return -1;

    spinlock_acquire(&space->lock);

    // Profile check
    if (space->timing_class >= VM_TIMING_FIRM_RT && (req->map_flags & VM_MAP_NO_LAZY_SYNC) == 0) {
        // RT constraints require strict sync
        // return -1; // Uncomment for strict enforcement
    }

    // Refactor to use Address Space Region tree for authority instead of mappings.head
    address_space_t *aspace = (address_space_t *)space;
    vm_region_t *r = NULL;
    int attach_ret = aspace_region_attach(aspace, req->va, req->len, req->prot, req->map_flags, VM_INHERIT_COPY_META, NULL, 0, &r);
    if (attach_ret != 0) {
        spinlock_release(&space->lock);
        return attach_ret;
    }

    space->generation++;

    bool is_strict = (req->map_flags & VM_MAP_NO_LAZY_SYNC) || (space->timing_class >= VM_TIMING_FIRM_RT);

    // Coordinate with Monitor Plane
    mon_vm_send_map(space, req, is_strict);

    spinlock_release(&space->lock);

    // If local core is active, realize immediately
    if (space->active_cores & (1ULL << hal_cpu_get_id())) {
        if (active_arch_vm_ops && active_arch_vm_ops->map) {
            vm_core_state_t local_state = { .core_id = hal_cpu_get_id() };
            active_arch_vm_ops->map(space, &local_state, req->va, req->pa, req->len, req->prot, req->mem_type, req->map_flags);
        }
    }

    return 0;
}

int vm_unmap(vm_space_t *space, uintptr_t va, size_t len) {
    if (!space) return -1;

    spinlock_acquire(&space->lock);

    address_space_t *aspace = (address_space_t *)space;
    int detach_ret = aspace_region_detach(aspace, va);
    if (detach_ret == 0) {
        space->generation++; // Bump generation for revoke
        // Coordinate with Monitor Plane (Unmap MUST be strict)
        mon_vm_send_unmap(space, va, len, true);
    }

    spinlock_release(&space->lock);

    if (detach_ret == 0) {
        // Local invalidation
        if (space->active_cores & (1ULL << hal_cpu_get_id())) {
            if (active_arch_vm_ops && active_arch_vm_ops->unmap) {
                vm_core_state_t local_state = { .core_id = hal_cpu_get_id() };
                active_arch_vm_ops->unmap(space, &local_state, va, len);
            }
        }
    }

    return detach_ret;
}

int vm_protect(vm_space_t *space, uintptr_t va, size_t len, uint64_t prot, uint64_t mem_type) {
    if (!space) return -1;

    spinlock_acquire(&space->lock);

    address_space_t *aspace = (address_space_t *)space;
    vm_region_t *r = aspace_lookup_region(aspace, va);
    if (r) {
        r->prot = prot;
        // mem_type currently not explicitly tracked in region, could be mapped to map_flags or new field
        space->generation++;
        // Downgrades should be strict, upgrades can be lazy. Assuming strict for now.
        mon_vm_send_protect(space, va, len, prot, mem_type, true);
    } else {
        spinlock_release(&space->lock);
        return -1;
    }

    spinlock_release(&space->lock);

    if (space->active_cores & (1ULL << hal_cpu_get_id())) {
        if (active_arch_vm_ops && active_arch_vm_ops->protect) {
            vm_core_state_t local_state = { .core_id = hal_cpu_get_id() };
            active_arch_vm_ops->protect(space, &local_state, va, len, prot, mem_type);
        }
    }

    return 0;
}
