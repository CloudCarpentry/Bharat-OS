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

    capability_table_t *table = (capability_table_t *)ctx->process->security_sandbox_ctx;
    capability_entry_t entry;

    kstatus_t st = cap_table_lookup(table, cap_id, expected_type, required_rights, &entry);
    if (st != K_OK) {
        return kstatus_to_bh_status(st);
    }

    if (entry.state != CAP_STATE_LIVE) {
        return BH_ERR_STALE_CAPABILITY;
    }

    return BH_OK;
}
