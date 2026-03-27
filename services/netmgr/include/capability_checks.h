#ifndef NETMGR_CAPABILITY_CHECKS_H
#define NETMGR_CAPABILITY_CHECKS_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/cap/cap.h>
#include <bharat/cap/cap_validate.h>

bool netmgr_cap_check_rights(
    bharat_cap_handle_t caller_cap,
    bharat_cap_object_type_t object_type,
    uint64_t object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope);

void netmgr_set_caller_cap(bharat_cap_handle_t cap);

#endif // NETMGR_CAPABILITY_CHECKS_H
