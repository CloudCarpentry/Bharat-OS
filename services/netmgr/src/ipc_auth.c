#include "ipc_auth.h"
#include <bharat/uapi/ipc/status.h>
#include <bharat/cap/cap_validate.h>

extern bharat_cap_handle_t current_caller_cap;

// Mock audit function
static void netmgr_audit_cap_failure(
    bharat_cap_handle_t caller_cap,
    bharat_cap_object_type_t object_type,
    uint64_t object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_status_t status)
{
    (void)caller_cap;
    (void)object_type;
    (void)object_id;
    (void)required_rights;
    (void)required_scope;
    (void)status;
    // In a real system, this would write to the audit log or telemetry
    // printf("AUDIT: Capability validation failed with status %d\n", status);
}

int netmgr_authorize(
    bharat_cap_handle_t caller_cap,
    bharat_cap_object_type_t object_type,
    uint64_t object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope)
{
    // Use passed capability or fallback to the mock thread-local token
    bharat_cap_handle_t cap_to_check = caller_cap;
    if (cap_to_check == BHARAT_CAP_INVALID_HANDLE) {
        cap_to_check = current_caller_cap;
    }

    bharat_cap_validation_result_t vr = {0};

    bharat_cap_status_t st = bharat_cap_validate(
        cap_to_check,
        object_type,
        object_id,
        required_rights,
        required_scope,
        &vr);

    if (st != BHARAT_CAP_OK || !vr.allowed) {
        netmgr_audit_cap_failure(cap_to_check, object_type, object_id,
                                 required_rights, required_scope, vr.status);
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    return BHARAT_IPC_STATUS_OK;
}
