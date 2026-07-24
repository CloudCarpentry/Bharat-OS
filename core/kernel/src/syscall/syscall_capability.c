#include "syscall/syscall_capability.h"
#include "capability.h"
#include "trap/syscall_status.h"

bh_status_t bh_syscall_validate_capability(bh_syscall_ctx_t *ctx,
                                           uint32_t cap_id,
                                           uint32_t expected_type,
                                           uint64_t required_rights) {
    if (!ctx || !ctx->process || !ctx->process->security_sandbox_ctx) {
        return BH_ERR_ACCESS_DENIED;
    }

    if (!bh_cap_is_valid_encoding(cap_id)) {
        return BH_ERR_BAD_CAPABILITY;
    }

    capability_table_t *table = (capability_table_t *)ctx->process->security_sandbox_ctx;
    cap_validation_request_t req = {
        .cap_id = cap_id,
        .expected_object_type = (cap_type_t)expected_type,
        .required_rights = required_rights,
        .requester_pid = ctx->process->process_id,
        .expected_generation = bh_cap_generation(cap_id)
    };

    capability_entry_t entry;
    kstatus_t st = cap_validate_ex(table, &req, &entry);
    if (st != K_OK) {
        return kstatus_to_bh_status(st);
    }

    return BH_OK;
}
