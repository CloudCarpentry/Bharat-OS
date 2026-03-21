#include "../../../../include/mm/mm_aspace_switch.h"
#include "../../../../include/bharat/cpu_local.h"
#include "../../../../include/kernel.h"
#include "../../../../include/panic.h"
#include "console/console_core.h"

void mm_switch_active_aspace(uint32_t core_id, address_space_t *prev_as, address_space_t *next_as) {
    if (core_id >= MAX_CPUS) return;

    if (prev_as == next_as) {
        // Just in case, ensure it's still current
        if (next_as) {
            g_cpu_locals[core_id].current_as = next_as;
            g_cpu_locals[core_id].current_as_id = next_as->object_id;
        } else {
            g_cpu_locals[core_id].current_as = NULL;
            g_cpu_locals[core_id].current_as_id = KERNEL_AS_ID;
        }
        return;
    }

    if (prev_as) {
        __atomic_and_fetch(&prev_as->active_mask, ~(1ULL << core_id), __ATOMIC_SEQ_CST);
    }

    if (next_as) {
        g_cpu_locals[core_id].current_as = next_as;
        g_cpu_locals[core_id].current_as_id = next_as->object_id;
        __atomic_or_fetch(&next_as->active_mask, (1ULL << core_id), __ATOMIC_SEQ_CST);
    } else {
        g_cpu_locals[core_id].current_as = NULL;
        g_cpu_locals[core_id].current_as_id = KERNEL_AS_ID;
    }

    // A full barrier to ensure software state is visible before any HW TLB/PT operations occur
    __asm__ volatile("" ::: "memory");

    // Now trigger the new protection domain activate routine
    if (next_as && next_as->prot_domain) {
        prot_domain_activate(next_as->prot_domain);
    }
}

void vm_debug_validate_active_tracking(void) {
    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        address_space_t *as = g_cpu_locals[i].current_as;
        if (as) {
            uint64_t mask = __atomic_load_n(&as->active_mask, __ATOMIC_ACQUIRE);
            if (!(mask & (1ULL << i))) {
                kernel_panic("vm_debug_validate_active_tracking: active_mask out of sync\n");
            }
        }
    }
}
