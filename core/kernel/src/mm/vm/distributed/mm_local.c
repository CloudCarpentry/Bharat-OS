#include "../../../../include/mm.h"
#include "../../../../include/mm/vm_space.h"
#include "../../../../include/mm/vm_mapping.h"
#include "../../../../include/monitor/mon_vm_ops.h"
#include "../../../../include/hal/hal.h"
#include "../../../../include/slab.h"
#include "../../../../include/mm/mm_remote.h"
#include "../../../../include/mm/mm_local.h"

// TLS or per-core flag to denote if we are currently handling a URPC message
// This would ideally be in a per-core data structure (e.g. core_local_data_t)
static __thread bool g_in_urpc_handler = false;

bool in_urpc_handler(void) {
    return g_in_urpc_handler;
}

uint32_t current_core_id(void) {
    return hal_cpu_get_id();
}

bool current_core_owns(address_space_t *as) {
    if (!as) return false;
    // Basic ownership check.
    // In actual implementation, `owner_core_id` tracks the core that has authoritative control.
    return as->owner_core_id == hal_cpu_get_id();
}

// Implement mock mm_local_* functions just to establish the boundary.
int mm_local_map(address_space_t *as, virt_addr_t va, phys_addr_t pa, uint32_t flags) {
    // Assert we own this AS locally or it's a safe local operation
    // KASSERT(current_core_owns(as));
    return mm_vmm_map_page(as, va, pa, flags);
}

int mm_local_unmap(address_space_t *as, virt_addr_t va) {
    // KASSERT(current_core_owns(as));
    return mm_vmm_unmap_page(as, va);
}

int mm_local_protect(address_space_t *as, virt_addr_t va, uint32_t flags) {
    // KASSERT(current_core_owns(as));
    (void)as;
    (void)va;
    (void)flags;
    return 0; // Stub for now
}
