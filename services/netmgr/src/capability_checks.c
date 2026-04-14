#include "capability_checks.h"
#include <bharat/cap/cap.h>
#include <bharat/cap/cap_validate.h>
#include <stddef.h>

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
