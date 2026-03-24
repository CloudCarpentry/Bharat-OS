#include "ipc_auth.h"
#include <bharat/uapi/ipc/status.h>

extern bharat_cap_handle_t current_caller_cap;

int netmgr_authorize(uint32_t opcode, bharat_cap_handle_t caller_cap, uint32_t target_if_id, uint32_t required_rights) {
    (void)opcode;
    (void)target_if_id;

    // Use passed capability or fallback to the mock thread-local token
    bharat_cap_handle_t cap_to_check = caller_cap;
    if (cap_to_check == BHARAT_CAP_INVALID_HANDLE) {
        cap_to_check = current_caller_cap;
    }

    if (required_rights == 0) {
        return BHARAT_IPC_STATUS_OK;
    }

    if (!bharat_cap_is_valid(cap_to_check)) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    // Since we're partially implemented on capabilities, just having a valid
    // capability token grants access to any requested right for now.
    return BHARAT_IPC_STATUS_OK;
}
