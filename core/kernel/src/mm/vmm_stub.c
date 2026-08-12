#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "kernel/status.h"
#include "mm/aspace.h"
#include "mm/vm_space.h"
#include "mm/pt_cache.h"
#include "mm/tlb.h"

/* Global fallback structures for non-VM / flat memory builds */
address_space_t kernel_space = {0};
vm_space_t *g_active_spaces[128] = {0};

/* Page table cache stubs */
void pt_cache_init(void) {
}

phys_addr_t pt_cache_alloc(void) {
    return 0;
}

void pt_cache_free(phys_addr_t pa) {
    (void)pa;
}

/* TLB stubs */
int tlb_init(void) {
    return K_OK;
}

void tlb_shootdown(vm_aspace_t *as, uint64_t vaddr) {
    (void)as;
    (void)vaddr;
}

void hal_tlb_invalidate_all(void) {
}

kstatus_t vmm_send_tlb_invalidate_ex(vm_aspace_t *aspace, uintptr_t va, size_t len, tlb_inv_kind_t type, tlb_failure_policy_t policy) {
    (void)aspace;
    (void)va;
    (void)len;
    (void)type;
    (void)policy;
    return K_OK;
}

/* VMM core stubs */
void vmm_process_urpc_messages(void) {
}

void vmm_process_local_urpc_messages(uint32_t core_id) {
    (void)core_id;
}

address_space_t *mm_create_address_space(void) {
    return &kernel_space;
}

int mm_vmm_map_page(address_space_t *aspace, uintptr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)aspace;
    (void)vaddr;
    (void)paddr;
    (void)flags;
    return K_OK;
}

int mm_vmm_unmap_page(address_space_t *aspace, uintptr_t vaddr) {
    (void)aspace;
    (void)vaddr;
    return K_OK;
}

int vmm_map_page(uintptr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)vaddr;
    (void)paddr;
    (void)flags;
    return K_OK;
}

int vmm_unmap_page(uintptr_t vaddr) {
    (void)vaddr;
    return K_OK;
}

phys_addr_t vmm_get_kernel_root(void) {
    return 0;
}

int mm_global_init(void) {
    return K_OK;
}

int mm_cpu_online(uint32_t cpu_id) {
    (void)cpu_id;
    return K_OK;
}

int vmm_is_kernel_space_ready(void) {
    return 1;
}

int vmm_handle_cow_fault(vm_aspace_t *as, uintptr_t vaddr) {
    (void)as;
    (void)vaddr;
    return K_ERR_UNSUPPORTED;
}

/* Address space region & lifecycle stubs */
int aspace_create(address_space_t **out_aspace, uint32_t flags) {
    (void)flags;
    if (out_aspace) {
        *out_aspace = &kernel_space;
    }
    return K_OK;
}

int aspace_destroy(address_space_t *aspace) {
    (void)aspace;
    return K_OK;
}

int aspace_clone(address_space_t *src, address_space_t **out_clone, uint32_t clone_flags) {
    (void)src;
    (void)clone_flags;
    if (out_clone) {
        *out_clone = &kernel_space;
    }
    return K_OK;
}

int mm_switch_active_aspace(address_space_t *aspace) {
    (void)aspace;
    return K_OK;
}

int aspace_region_reserve(address_space_t *aspace,
                          uintptr_t base,
                          size_t length,
                          uint32_t prot,
                          uint32_t map_flags,
                          vm_inherit_t inherit,
                          vm_region_t **out_region) {
    (void)aspace;
    (void)base;
    (void)length;
    (void)prot;
    (void)map_flags;
    (void)inherit;
    if (out_region) {
        *out_region = NULL;
    }
    return K_OK;
}

int aspace_region_detach(address_space_t *aspace, uintptr_t base) {
    (void)aspace;
    (void)base;
    return K_OK;
}

void aspace_mark_poisoned(address_space_t *aspace) {
    if (aspace) {
        aspace->state = ASPACE_STATE_POISONED;
    }
}
