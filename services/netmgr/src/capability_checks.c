#include "capability_checks.h"
#include <bharat/cap/cap.h>
#include <bharat/cap/cap_validate.h>
#include <stddef.h>

// A mock thread-local or static capability token to represent the caller's context.
// In a real system, this would be fetched from the IPC message header or thread context.
bharat_cap_handle_t current_caller_cap = BHARAT_CAP_INVALID_HANDLE;

void netmgr_set_caller_cap(bharat_cap_handle_t cap) {
    current_caller_cap = cap;
}

bool netmgr_cap_check_rights(
    bharat_cap_handle_t caller_cap,
    bharat_cap_object_type_t object_type,
    uint64_t object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope)
{
    bharat_cap_validation_result_t vr = {0};

    if (bharat_cap_validate(caller_cap,
                            object_type,
                            object_id,
                            required_rights,
                            required_scope,
                            &vr) != BHARAT_CAP_OK) {
        return false;
    }

    return vr.allowed;
}
