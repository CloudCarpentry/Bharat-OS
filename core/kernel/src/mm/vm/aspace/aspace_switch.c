#include "../../../../include/mm/mm_aspace_switch.h"
#include "../../../../include/bharat/cpu_local.h"
#include "../../../../include/kernel.h"
#include "../../../../include/panic.h"
#include "console/console_core.h"
#include "../../../../include/mm/tlb_internal.h"

void mm_switch_active_aspace(uint32_t core_id, address_space_t *prev_as, address_space_t *next_as) {
    if (core_id >= MAX_CPUS) return;

    console_write_raw("[ASPACE_SWITCH_BEGIN]\n", 22);

    if (prev_as == next_as) {
        if (next_as) {
            g_cpu_locals[core_id].current_as = next_as;
            g_cpu_locals[core_id].current_as_id = next_as->object_id;
            g_tlb_cpu_state[core_id].active_aspace = next_as;
        } else {
            g_cpu_locals[core_id].current_as = NULL;
            g_cpu_locals[core_id].current_as_id = KERNEL_AS_ID;
            g_tlb_cpu_state[core_id].active_aspace = NULL;
        }
        return;
    }

    if (prev_as) {
        console_write_raw("[ASPACE_DEACTIVATE]\n", 20);
        aspace_deactivate_on_cpu(prev_as, core_id);
    }

    if (next_as) {
        g_cpu_locals[core_id].current_as = next_as;
        g_cpu_locals[core_id].current_as_id = next_as->object_id;
        g_tlb_cpu_state[core_id].active_aspace = next_as;
        console_write_raw("[ASPACE_ACTIVATE_CPU]\n", 22);
        aspace_activate_on_cpu(next_as, core_id);
    } else {
        g_cpu_locals[core_id].current_as = NULL;
        g_cpu_locals[core_id].current_as_id = KERNEL_AS_ID;
        g_tlb_cpu_state[core_id].active_aspace = NULL;
    }

    __asm__ volatile("" ::: "memory");

    if (next_as && next_as->prot_domain) {
        console_write_raw("[PROT_DOMAIN_ACTIVATE_CALL]\n", 28);
        prot_domain_activate(next_as->prot_domain);
        console_write_raw("[PROT_DOMAIN_ACTIVATE_DONE]\n", 28);
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
