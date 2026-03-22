#include "../../include/mm/tlb.h"
#include "../../include/mm/tlb_internal.h"
#include "../../include/hal/hal_tlb.h"
#include "../../include/hal/hal.h"
#include "../../include/bharat/cpu_local.h"

int tlb_invalidate_local(vm_aspace_t *aspace, uintptr_t va, size_t len, tlb_inv_kind_t kind) {
    if (!aspace || !active_hal_tlb) return -1;

    uint32_t current_core = hal_cpu_get_id();

    // Check if we are running the target aspace locally
    if (g_cpu_locals[current_core].current_as_id != aspace->object_id) {
        return 0; // No need to local flush if not active
    }

    const hal_tlb_caps_t *caps = hal_tlb_caps();
    const bool can_page = caps && caps->supports_page_flush;
    const bool can_range = caps && caps->supports_range_flush;
    const bool can_aspace = caps && caps->supports_aspace_flush;

    if (kind == TLB_INV_PAGE && can_page && active_hal_tlb->flush_page_local) {
        active_hal_tlb->flush_page_local(va);
        g_tlb_cpu_state[current_core].page_flushes++;
    } else if (kind == TLB_INV_RANGE && can_range && active_hal_tlb->flush_range_local) {
        active_hal_tlb->flush_range_local(va, len);
        g_tlb_cpu_state[current_core].range_flushes++;
    } else if ((kind == TLB_INV_ASPACE || kind == TLB_INV_FULL) && can_aspace && active_hal_tlb->flush_asid_local) {
        active_hal_tlb->flush_asid_local(aspace->object_id & 0xFFFF);
        g_tlb_cpu_state[current_core].aspace_flushes++;
    } else if (active_hal_tlb->flush_all_local) {
        active_hal_tlb->flush_all_local();
        g_tlb_cpu_state[current_core].full_flushes++;
    }

    g_tlb_cpu_state[current_core].local_flushes++;
    return 0;
}

// Ensure the hal_tlb interface also uses the local tracking correctly
void hal_tlb_invalidate_page(address_space_t *aspace, virt_addr_t va) {
    tlb_invalidate_all(aspace, va, PAGE_SIZE, TLB_INV_PAGE);
}

void hal_tlb_invalidate_range(address_space_t *aspace, virt_addr_t start, size_t len) {
    tlb_invalidate_all(aspace, start, len, TLB_INV_RANGE);
}

void hal_tlb_invalidate_aspace(address_space_t *aspace) {
    tlb_invalidate_all(aspace, 0, 0, TLB_INV_ASPACE);
}

void hal_tlb_invalidate_all(void) {
    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        if (g_cpu_locals[i].current_as) {
             tlb_invalidate_all(g_cpu_locals[i].current_as, 0, 0, TLB_INV_FULL);
        }
    }
}
