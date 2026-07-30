#include "../../../../include/mm/vm_space.h"
#include "../../../../include/mm/mem_model.h"
#include "../../../../include/mm/arch_vm.h"
#include "../../../../include/mm.h"
#include "../../../../include/monitor/mon_vm_ops.h"
#include "../../../../include/hal/hal.h"
#include "../../../../include/slab.h"
#include "../../../../include/spinlock.h"
#include "../../../../include/bharat/cpu_local.h"
#include "../../../../include/mm/mm_aspace_switch.h"
#include "../../../../include/mm/prot_domain.h"
#include <stddef.h>
#include <stdint.h>

// Pre-allocated active spaces database (mock)
vm_space_t* g_active_spaces[128] = {0};

#ifndef hal_get_core_id
extern uint32_t hal_get_core_id(void);
#endif

#ifndef spinlock_acquire
#define spinlock_acquire spin_lock
#endif
#ifndef spinlock_release
#define spinlock_release spin_unlock
#endif
#ifndef spinlock_init
#define spinlock_init spin_lock_init
#endif

const arch_vm_ops_t* active_arch_vm_ops = NULL;

static uint64_t next_space_id = 1;

int vm_space_create(vm_space_t **out, mem_profile_t profile, vm_timing_class_t timing) {
    if (!out) return -1;

    mem_model_t current_model = mem_model_get_current();
    if (profile == MEM_PROFILE_MPU_ONLY && current_model != MEM_MODEL_MPU) {
        return -1;
    }
    if (profile != MEM_PROFILE_MPU_ONLY && current_model == MEM_MODEL_MPU) {
        return -1;
    }

    vm_space_t *space = (vm_space_t *)kmalloc(sizeof(vm_space_t));
    if (!space) return -1;

    spinlock_init(&space->lock);
    space->space_id = next_space_id++;
    space->generation = 1;
    space->profile = profile;
    space->timing_class = timing;
    space->flags = 0;
    space->rt_flags = 0;

    // Call canonical creator
    address_space_t *as = NULL;
    int as_ret = aspace_create(&as, 0);
    if (as_ret != K_OK) {
        kfree(space);
        return -1;
    }
    as->timing_class = timing;
    space->aspace = as;

    space->owner_cap.generation = 0;
    space->owner_cap.slot = 0;
    space->owner_cap.table = NULL;

    space->regions.root = NULL;
    space->mappings.head = NULL;
    space->allowed_cores = ~0ULL;
    space->active_cores = 0;
    space->realized_cores = 0;
    space->pending_cores = 0;
    space->rt_ready_cores = 0;
    space->home_monitor = hal_cpu_get_id();

    space->require_prefault = (timing >= VM_TIMING_FIRM_RT);
    space->allow_lazy_realize = (timing < VM_TIMING_FIRM_RT);
    space->allow_runtime_pt_alloc = (timing < VM_TIMING_HARD_RT);
    space->allow_remote_fault_recovery = (timing < VM_TIMING_HARD_RT);
    space->allow_demand_paging = (timing <= VM_TIMING_SOFT_RT);

    // Register into active spaces database
    for (int i = 0; i < 128; i++) {
        if (!g_active_spaces[i]) {
            g_active_spaces[i] = space;
            break;
        }
    }

    *out = space;
    return 0;
}

int vm_space_destroy(vm_space_t *space) {
    if (!space) return -1;

    spinlock_acquire(&space->lock);

    // Remove from active spaces database
    for (int i = 0; i < 128; i++) {
        if (g_active_spaces[i] == space) {
            g_active_spaces[i] = NULL;
            break;
        }
    }

    if (space->aspace) {
        aspace_destroy(space->aspace);
        space->aspace = NULL;
    }

    spinlock_release(&space->lock);
    kfree(space);
    return 0;
}

int vm_realize_on_core(vm_space_t *space, uint32_t core_id, bool strict) {
    (void)strict;
    if (!space || !space->aspace) return -1;

    prot_domain_t *pd = space->aspace->prot_domain;
    if (!pd) return -1;

    // Realize by walking the authoritative regions list
    vm_region_t *curr = space->aspace->regions;
    while (curr) {
        phys_addr_t pa = 0;
        if (curr->object && curr->object->kind == VM_OBJECT_DEVICE) {
            pa = curr->object->u.device.phys_base + curr->object_offset;
        } else if (curr->object && curr->object->kind == VM_OBJECT_DMA) {
            pa = curr->object->u.dma.phys_base + curr->object_offset;
        }

        // Execute hardware programming on the core
        prot_domain_map_region(pd, curr->base, pa, curr->length, curr->prot);
        curr = curr->next;
    }

    space->realized_cores |= (1ULL << core_id);
    return 0;
}

int vm_prepare_rt_core(vm_space_t *space, uint32_t core_id) {
    if (!space) return -1;

    if (space->timing_class != VM_TIMING_HARD_RT && space->timing_class != VM_TIMING_FIRM_RT) {
        return -1; // Not an RT space
    }

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

    if (space->aspace && space->aspace->prot_domain) {
        prot_domain_activate(space->aspace->prot_domain);

        // Strictly ordered update to software active_aspace before hardware switch
        address_space_t *prev = g_cpu_locals[core_id].current_as;
        mm_switch_active_aspace(core_id, prev, space->aspace);
    }

    spinlock_acquire(&space->lock);
    if (space->aspace) {
        space->active_cores = aspace_get_active_mask(space->aspace);
    } else {
        space->active_cores |= (1ULL << core_id);
    }
    spinlock_release(&space->lock);

    return 0;
}
