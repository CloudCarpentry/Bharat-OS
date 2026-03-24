#include "capability_checks.h"
#include <bharat/cap/cap.h>
#include <stddef.h>

// A mock thread-local or static capability token to represent the caller's context.
// In a real system, this would be fetched from the IPC message header or thread context.
bharat_cap_handle_t current_caller_cap = BHARAT_CAP_INVALID_HANDLE;

void netmgr_set_caller_cap(bharat_cap_handle_t cap) {
    current_caller_cap = cap;
}

bool netmgr_cap_check_rights(uint32_t required_rights, uint32_t object_id) {
    (void)object_id;
    (void)required_rights;

    // Strict fail-closed shim: deny by default, allow only when an explicit capability token/context is present.
    if (!bharat_cap_is_valid(current_caller_cap)) {
        return false;
    }

    // In a complete capability system, we would:
    // 1. Look up the capability object corresponding to 'current_caller_cap'.
    // 2. Check if the capability grants 'required_rights'.
    // 3. Check if the capability scope matches 'object_id' (e.g., specific interface or global).
    // For now, since the capability core is partial, possessing a valid token grants access.

    return true;
}
