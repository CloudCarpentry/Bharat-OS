#include "mm/vm_cap_dispatch.h"
#include "capability.h"
#include "sched/sched.h"
#include "hal/hal.h"
#include "syscall/usercopy.h"
#include "trap/syscall_status.h"
#include "mm/aspace.h"
#include "mm/prot_domain.h"
#include "mm/vm_mapping.h"
#include <bharat/uapi/system/vmm.h>

/* Weak fallbacks so standalone unit tests that only link vm_cap_dispatch.c
 * can override or fall back cleanly without pulling the full MM subsystem. */
kstatus_t __attribute__((weak)) cap_validate_and_pin(capability_table_t *table,
                                                     const cap_validation_request_t *req,
                                                     capability_entry_t *out_entry,
                                                     uint32_t *out_pinned_slot) {
    if (!out_pinned_slot) return K_ERR_INVALID_ARG;
    *out_pinned_slot = 0U;
    return cap_validate_ex(table, req, out_entry);
}

void __attribute__((weak)) cap_unpin(capability_table_t *table, uint32_t pinned_slot) {
    (void)table;
    (void)pinned_slot;
}

#if !defined(BHARAT_PROFILE_MPU_ONLY) && !defined(PROFILE_MPU_ONLY)
int __attribute__((weak)) aspace_region_reserve(address_space_t *aspace,
                                                uintptr_t base,
                                                size_t length,
                                                uint32_t prot,
                                                uint32_t map_flags,
                                                vm_inherit_t inherit,
                                                vm_region_t **out_region) {
    (void)aspace; (void)base; (void)length; (void)prot; (void)map_flags; (void)inherit; (void)out_region;
    return K_ERR_UNSUPPORTED;
}

int __attribute__((weak)) aspace_region_detach(address_space_t *aspace, uintptr_t base) {
    (void)aspace; (void)base;
    return K_ERR_UNSUPPORTED;
}

vm_region_t * __attribute__((weak)) aspace_lookup_region(address_space_t *aspace, uintptr_t va) {
    (void)aspace; (void)va;
    return NULL;
}

int __attribute__((weak)) prot_domain_map_region(prot_domain_t *domain,
                                                 uintptr_t vaddr,
                                                 uintptr_t paddr,
                                                 size_t size,
                                                 uint32_t flags) {
    (void)domain; (void)vaddr; (void)paddr; (void)size; (void)flags;
    return K_ERR_UNSUPPORTED;
}

int __attribute__((weak)) prot_domain_unmap_region(prot_domain_t *domain,
                                                   uintptr_t vaddr,
                                                   size_t size) {
    (void)domain; (void)vaddr; (void)size;
    return K_ERR_UNSUPPORTED;
}

int __attribute__((weak)) prot_domain_protect_region(prot_domain_t *domain,
                                                     uintptr_t vaddr,
                                                     size_t size,
                                                     uint32_t flags) {
    (void)domain; (void)vaddr; (void)size; (void)flags;
    return K_ERR_UNSUPPORTED;
}

int __attribute__((weak)) prot_domain_query_region(prot_domain_t *domain,
                                                   uintptr_t vaddr,
                                                   uintptr_t *paddr,
                                                   uint32_t *flags) {
    (void)domain; (void)vaddr; (void)paddr; (void)flags;
    return K_ERR_UNSUPPORTED;
}
#endif

static uint32_t uapi_prot_to_vm_prot(uint64_t uapi_prot) {
    uint32_t vm_prot = VM_PROT_USER;
    if (uapi_prot & BH_VM_PROT_READ) vm_prot |= VM_PROT_READ;
    if (uapi_prot & BH_VM_PROT_WRITE) vm_prot |= VM_PROT_WRITE;
    if (uapi_prot & BH_VM_PROT_EXEC) vm_prot |= VM_PROT_EXEC;
    return vm_prot;
}

kstatus_t vm_space_cap_invoke(uintptr_t cap_id, uintptr_t opcode,
                              uintptr_t request, uintptr_t reserved) {
    bh_process_t *caller = sched_current_process();
    if (!caller || !caller->security_sandbox_ctx || cap_id > UINT32_MAX ||
        !bh_cap_is_valid_encoding((uint32_t)cap_id)) return K_ERR_CAP_INVALID;
    capability_table_t *table = caller->security_sandbox_ctx;
    if (table->owner_pid != caller->process_id ||
        table->owner_core != hal_cpu_get_id()) return K_ERR_CAP_OWNERSHIP;

    cap_rights_mask_t rights;
    switch (opcode) {
    case BH_VM_OP_CREATE: rights = CAP_RIGHT_RESOURCE_ALLOC; break;
    case BH_VM_OP_DESTROY: rights = CAP_RIGHT_VMM_MANAGE; break;
    case BH_VM_OP_MAP: rights = CAP_RIGHT_MEMORY_MAP; break;
    case BH_VM_OP_UNMAP: rights = CAP_RIGHT_MEMORY_UNMAP; break;
    case BH_VM_OP_PROTECT: rights = CAP_RIGHT_VMM_MANAGE; break;
    case BH_VM_OP_QUERY: rights = CAP_RIGHT_QUERY; break;
    default: return K_ERR_UNSUPPORTED;
    }
    cap_validation_request_t validation = {
        .cap_id = (uint32_t)cap_id,
        .expected_object_type = opcode == BH_VM_OP_CREATE ? CAP_TYPE_PROCESS : CAP_TYPE_VM_SPACE,
        .required_rights = rights,
        .requester_pid = caller->process_id,
        .expected_generation = bh_cap_generation((uint32_t)cap_id)
    };
    capability_entry_t entry;
    uint32_t pinned_slot = 0U;
    kstatus_t status = cap_validate_and_pin(table, &validation, &entry, &pinned_slot);
    if (status != K_OK) return status;

    if (opcode == BH_VM_OP_CREATE && entry.object_ref != (uint64_t)(uintptr_t)caller) {
        cap_unpin(table, pinned_slot);
        return K_ERR_CAP_OWNERSHIP;
    }
    if (reserved) {
        cap_unpin(table, pinned_slot);
        return K_ERR_INVALID_ARG;
    }
    bh_vm_request_t req;
    bh_status_t copy = bh_copy_from_user(&req, (const void *)request, sizeof(req));
    if (copy != BH_OK) {
        cap_unpin(table, pinned_slot);
        return bh_status_to_kstatus(copy);
    }
    if (req.version != BH_VM_ABI_VERSION || req.struct_size != sizeof(req) ||
        req.reserved || req.profile > 3 || req.timing_class > 3 ||
        (req.protection & ~(BH_VM_PROT_READ | BH_VM_PROT_WRITE | BH_VM_PROT_EXEC))) {
        cap_unpin(table, pinned_slot);
        return K_ERR_INVALID_ARG;
    }
    if ((req.protection & (BH_VM_PROT_WRITE | BH_VM_PROT_EXEC)) ==
        (BH_VM_PROT_WRITE | BH_VM_PROT_EXEC)) {
        cap_unpin(table, pinned_slot);
        return K_ERR_VM_PROT;
    }
    if (req.address > UINTPTR_MAX || req.length > (uint64_t)(size_t)-1 ||
        req.length > UINTPTR_MAX - req.address ||
        req.length > UINT64_MAX - req.backing_offset) {
        cap_unpin(table, pinned_slot);
        return K_ERR_OVERFLOW;
    }
    if (opcode == BH_VM_OP_MAP || opcode == BH_VM_OP_UNMAP || opcode == BH_VM_OP_PROTECT) {
        if (!req.length) {
            cap_unpin(table, pinned_slot);
            return K_ERR_INVALID_ARG;
        }
        if ((req.address | req.length | req.backing_offset) & (PAGE_SIZE - 1U)) {
            cap_unpin(table, pinned_slot);
            return K_ERR_ALIGNMENT;
        }
    }

#if defined(BHARAT_PROFILE_MPU_ONLY) || defined(PROFILE_MPU_ONLY)
    cap_unpin(table, pinned_slot);
    return K_ERR_UNSUPPORTED;
#else
    if (opcode == BH_VM_OP_CREATE || opcode == BH_VM_OP_DESTROY || entry.object_ref == 0U) {
        cap_unpin(table, pinned_slot);
        return K_ERR_UNSUPPORTED;
    }

    address_space_t *aspace = (address_space_t *)(uintptr_t)entry.object_ref;
    if (aspace->owner_core_id != hal_cpu_get_id()) {
        cap_unpin(table, pinned_slot);
        return K_ERR_CAP_OWNERSHIP;
    }
    if (aspace->state == ASPACE_STATE_DYING ||
        aspace->state == ASPACE_STATE_DESTROYED ||
        aspace->state == ASPACE_STATE_POISONED) {
        cap_unpin(table, pinned_slot);
        return K_ERR_BAD_STATE;
    }
    if (!aspace->prot_domain) {
        cap_unpin(table, pinned_slot);
        return K_ERR_UNSUPPORTED;
    }
    if (req.address < aspace->user_base ||
        (aspace->user_limit != 0U && req.address + req.length > aspace->user_limit + 1ULL)) {
        cap_unpin(table, pinned_slot);
        return K_ERR_INVALID_ARG;
    }

    if (opcode == BH_VM_OP_MAP) {
        if (!bh_cap_is_valid_encoding(req.backing_cap)) {
            cap_unpin(table, pinned_slot);
            return K_ERR_CAP_INVALID;
        }
        cap_rights_mask_t backing_rights = CAP_RIGHT_MEMORY_MAP;
        cap_validation_request_t backing_val = {
            .cap_id = req.backing_cap,
            .expected_object_type = CAP_TYPE_NONE,
            .required_rights = backing_rights,
            .requester_pid = caller->process_id,
            .expected_generation = bh_cap_generation(req.backing_cap)
        };
        capability_entry_t backing_entry;
        uint32_t backing_pinned_slot = 0U;
        status = cap_validate_and_pin(table, &backing_val, &backing_entry, &backing_pinned_slot);
        if (status != K_OK) {
            cap_unpin(table, pinned_slot);
            return status;
        }
        if (backing_entry.type != CAP_TYPE_MEMORY && backing_entry.type != CAP_TYPE_HMEM) {
            cap_unpin(table, backing_pinned_slot);
            cap_unpin(table, pinned_slot);
            return K_ERR_CAP_WRONG_TYPE;
        }
        if (backing_entry.object_ref == 0U) {
            cap_unpin(table, backing_pinned_slot);
            cap_unpin(table, pinned_slot);
            return K_ERR_CAP_INVALID;
        }

        uintptr_t phys_base = (uintptr_t)backing_entry.object_ref;
        uint64_t backing_size = 0U;
        if (backing_entry.scope_id != 0U) {
            backing_size = (uint64_t)backing_entry.scope_id;
        } else if (backing_entry.flags != 0U) {
            backing_size = (uint64_t)backing_entry.flags;
        } else {
            backing_size = PAGE_SIZE;
        }

        /* Enforce real backing extent: [backing_offset, backing_offset + length) must fit. */
        if (req.backing_offset > backing_size ||
            req.length > backing_size - req.backing_offset) {
            cap_unpin(table, backing_pinned_slot);
            cap_unpin(table, pinned_slot);
            return K_ERR_OVERFLOW;
        }

        uint32_t vm_prot = uapi_prot_to_vm_prot(req.protection);
        vm_region_t *region = NULL;
        int r_st = aspace_region_reserve(aspace, (uintptr_t)req.address, (size_t)req.length,
                                         vm_prot, VM_MAP_FIXED, VM_INHERIT_NONE, &region);
        if (r_st != K_OK) {
            cap_unpin(table, backing_pinned_slot);
            cap_unpin(table, pinned_slot);
            return (kstatus_t)r_st;
        }

        int m_st = prot_domain_map_region(aspace->prot_domain,
                                          (uintptr_t)req.address,
                                          (uintptr_t)(phys_base + req.backing_offset),
                                          (size_t)req.length,
                                          vm_prot);
        if (m_st != K_OK) {
            (void)aspace_region_detach(aspace, (uintptr_t)req.address);
            cap_unpin(table, backing_pinned_slot);
            cap_unpin(table, pinned_slot);
            return (kstatus_t)m_st;
        }

        cap_unpin(table, backing_pinned_slot);
        cap_unpin(table, pinned_slot);
        return K_OK;
    }

    if (opcode == BH_VM_OP_UNMAP) {
        vm_region_t *region = aspace_lookup_region(aspace, (uintptr_t)req.address);
        if (!region || region->base != (uintptr_t)req.address ||
            region->length != (size_t)req.length) {
            cap_unpin(table, pinned_slot);
            return K_ERR_VM_UNMAPPED;
        }
        int u_st = prot_domain_unmap_region(aspace->prot_domain,
                                            (uintptr_t)req.address,
                                            (size_t)req.length);
        if (u_st != K_OK) {
            cap_unpin(table, pinned_slot);
            return (kstatus_t)u_st;
        }
        int d_st = aspace_region_detach(aspace, (uintptr_t)req.address);
        cap_unpin(table, pinned_slot);
        return (kstatus_t)d_st;
    }

    if (opcode == BH_VM_OP_PROTECT) {
        vm_region_t *region = aspace_lookup_region(aspace, (uintptr_t)req.address);
        if (!region || region->base != (uintptr_t)req.address ||
            region->length != (size_t)req.length) {
            cap_unpin(table, pinned_slot);
            return K_ERR_VM_UNMAPPED;
        }
        uint32_t vm_prot = uapi_prot_to_vm_prot(req.protection);
        int p_st = prot_domain_protect_region(aspace->prot_domain,
                                              (uintptr_t)req.address,
                                              (size_t)req.length,
                                              vm_prot);
        if (p_st != K_OK) {
            cap_unpin(table, pinned_slot);
            return (kstatus_t)p_st;
        }
        region->prot = vm_prot;
        cap_unpin(table, pinned_slot);
        return K_OK;
    }

    if (opcode == BH_VM_OP_QUERY) {
        vm_region_t *region = aspace_lookup_region(aspace, (uintptr_t)req.address);
        if (!region) {
            cap_unpin(table, pinned_slot);
            return K_ERR_VM_UNMAPPED;
        }
        uintptr_t paddr = 0U;
        uint32_t pd_prot = 0U;
        int q_st = prot_domain_query_region(aspace->prot_domain,
                                            (uintptr_t)req.address,
                                            &paddr,
                                            &pd_prot);
        cap_unpin(table, pinned_slot);
        return (kstatus_t)q_st;
    }

    cap_unpin(table, pinned_slot);
    return K_ERR_UNSUPPORTED;
#endif
}
