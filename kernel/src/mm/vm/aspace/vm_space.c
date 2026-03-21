#include "../../../../include/mm/vm_space.h"
#include "../../../../include/mm/arch_vm.h"
#include "../../../../include/mm.h"
#include "../../../../include/monitor/mon_vm_ops.h"
#include "../../../../include/hal/hal.h"
#include "../../../../include/slab.h"
#include "../../../../include/spinlock.h"
#include "../../../../include/bharat/cpu_local.h"
#include "../../../../include/mm/mm_aspace_switch.h"
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
#ifndef spinlock_init
#define spinlock_init spin_lock_init
#endif
#include <stdint.h>

const arch_vm_ops_t* active_arch_vm_ops = NULL;

static uint64_t next_space_id = 1;

int vm_space_create(vm_space_t **out, mem_profile_t profile, vm_timing_class_t timing) {
    if (!out) return -1;

    vm_space_t *space = (vm_space_t *)kmalloc(sizeof(vm_space_t));
    if (!space) return -1;

    spinlock_init(&space->lock);
    space->space_id = next_space_id++;
    space->generation = 1;
    space->profile = profile;
    space->timing_class = timing;
    space->flags = 0;
    space->rt_flags = 0;

    // Initialize cap handle to 0
    space->owner_cap.generation = 0;
    space->owner_cap.slot = 0;
    space->owner_cap.table = NULL;

    space->regions.root = NULL;
    space->mappings.head = NULL;
    space->allowed_cores = ~0ULL; // Allow all by default for MVP
    space->active_cores = 0;
    space->realized_cores = 0;
    space->pending_cores = 0;
    space->rt_ready_cores = 0;
    space->home_monitor = hal_cpu_get_id();

    // Timing-specific policies
    space->require_prefault = (timing >= VM_TIMING_FIRM_RT);
    space->allow_lazy_realize = (timing < VM_TIMING_FIRM_RT);
    space->allow_runtime_pt_alloc = (timing < VM_TIMING_HARD_RT);
    space->allow_remote_fault_recovery = (timing < VM_TIMING_HARD_RT);
    space->allow_demand_paging = (timing <= VM_TIMING_SOFT_RT);

    *out = space;
    return 0;
}

int vm_space_destroy(vm_space_t *space) {
    if (!space) return -1;

    spinlock_acquire(&space->lock);

    // DEPRECATED: Legacy registry clean up
    vm_mapping_t *curr = space->mappings.head;
    while (curr) {
        vm_mapping_t *next = curr->next;
        kfree(curr);
        curr = next;
    }
    space->mappings.head = NULL;

    // TODO: Send MON_VM_SPACE_DESTROY to clean up remote realizations
    // Note: True tear down logic should iterate `space->regions` but this object
    // itself is a legacy distributed address space concept.

    spinlock_release(&space->lock);
    kfree(space);
    return 0;
}

int vm_realize_on_core(vm_space_t *space, uint32_t core_id, bool strict) {
    (void)strict;
    if (!space) return -1;

    // Triggered locally or remotely to force a realization sync
    // In a full implementation, this walks `space->mappings` and calls `arch_vm_ops_t->map`.

    if (active_arch_vm_ops && active_arch_vm_ops->space_init) {
        vm_core_state_t local_state = {0};
        local_state.core_id = core_id;
        // Mock init
        active_arch_vm_ops->space_init(space, &local_state);

        // Walk mappings and realize them (DEPRECATED PATH)
        // Correct path is lazy faults or walk address_space_t -> regions.
        vm_mapping_t *curr = space->mappings.head;
        while (curr) {
            active_arch_vm_ops->map(space, &local_state, curr->va_start, curr->pa_start, curr->length, curr->prot, curr->mem_type, curr->flags);
            curr = curr->next;
        }

        space->realized_cores |= (1ULL << core_id);
    }

    return 0;
}

int vm_prepare_rt_core(vm_space_t *space, uint32_t core_id) {
    if (!space) return -1;

    if (space->timing_class != VM_TIMING_HARD_RT && space->timing_class != VM_TIMING_FIRM_RT) {
        return -1; // Not an RT space
    }

    // Fully pre-realize and validate
    int ret = vm_realize_on_core(space, core_id, true);
    if (ret == 0) {
        spinlock_acquire(&space->lock);
        space->rt_ready_cores |= (1ULL << core_id);
        spinlock_release(&space->lock);
    }

    return ret;
}

int vm_activate_local(vm_space_t *space) {
    if (!space) return -1;

    uint32_t core_id = hal_cpu_get_id();

    // Ensure we are realized
    if (!(space->realized_cores & (1ULL << core_id))) {
        if (!space->allow_lazy_realize) {
            return -1; // Fatal for Firm/Hard RT
        }
        vm_realize_on_core(space, core_id, false);
    }

    if (active_arch_vm_ops && active_arch_vm_ops->activate) {
        vm_core_state_t local_state = {0};
        local_state.core_id = core_id;
        // Strictly ordered update to software active_aspace before hardware switch
        address_space_t *prev = g_cpu_locals[core_id].current_as;
        mm_switch_active_aspace(core_id, prev, (address_space_t *)space);
        active_arch_vm_ops->activate(space, &local_state);
    }

    spinlock_acquire(&space->lock);
    space->active_cores |= (1ULL << core_id);
    spinlock_release(&space->lock);

    return 0;
}
