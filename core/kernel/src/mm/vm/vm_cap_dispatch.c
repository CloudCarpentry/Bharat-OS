#include "mm/vm_cap_dispatch.h"
#include "capability.h"
#include "sched/sched.h"
#include "hal/hal.h"
#include "syscall/usercopy.h"
#include "trap/syscall_status.h"
#include <bharat/uapi/system/vmm.h>

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
    kstatus_t status = cap_validate_ex(table, &validation, &entry);
    if (status != K_OK) return status;
    if (opcode == BH_VM_OP_CREATE && entry.object_ref != (uint64_t)(uintptr_t)caller)
        return K_ERR_CAP_OWNERSHIP;
    if (reserved) return K_ERR_INVALID_ARG;
    bh_vm_request_t req;
    bh_status_t copy = bh_copy_from_user(&req, (const void *)request, sizeof(req));
    if (copy != BH_OK) return bh_status_to_kstatus(copy);
    if (req.version != BH_VM_ABI_VERSION || req.struct_size != sizeof(req) ||
        req.reserved || req.profile > 3 || req.timing_class > 3 ||
        (req.protection & ~(BH_VM_PROT_READ | BH_VM_PROT_WRITE | BH_VM_PROT_EXEC)))
        return K_ERR_INVALID_ARG;
    if ((req.protection & (BH_VM_PROT_WRITE | BH_VM_PROT_EXEC)) ==
        (BH_VM_PROT_WRITE | BH_VM_PROT_EXEC)) return K_ERR_VM_PROT;
    if (req.address > UINTPTR_MAX || req.length > SIZE_MAX ||
        req.length > UINTPTR_MAX - req.address ||
        req.length > UINT64_MAX - req.backing_offset) return K_ERR_OVERFLOW;

    /* cap_validate_ex returns a snapshot, not a pinned object. Never dereference
     * object_ref or mutate VM state until a revoke-safe lifetime protocol exists.
     * cap_lookup_memory's synthetic 4096 extent is also insufficient MAP authority.
     * This path is deliberately truthful for every backend, including MPU-only. */
    return K_ERR_UNSUPPORTED;
}
