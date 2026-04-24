#include "ipc_auth.h"
#include <bharat/cap/cap_validate.h>
#include <bharat/namesvc/namesvc_ipc.h>
#include <bharat/uapi/ipc/status.h>

static uint64_t namesvc_required_rights(uint32_t opcode) {
    switch (opcode) {
        case NAMESVC_OP_LOOKUP:
            return BHARAT_CAP_RIGHT_READ;
        case NAMESVC_OP_REGISTER:
        case NAMESVC_OP_REMOVE:
        case NAMESVC_OP_LIST_INTERFACES:
            return BHARAT_CAP_RIGHT_WRITE;
        default:
            return 0;
    }
}

int namesvc_authorize(uint32_t opcode, bharat_cap_handle_t caller_cap) {
    if (caller_cap == BHARAT_CAP_INVALID_HANDLE) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    uint64_t required_rights = namesvc_required_rights(opcode);
    if (required_rights == 0) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    bharat_cap_scope_t required_scope = {
        .kind = BHARAT_CAP_SCOPE_SERVICE,
        .scope_id = 0
    };

    bharat_cap_validation_result_t vr = {0};
    bharat_cap_status_t st = bharat_cap_validate(
        caller_cap,
        BHARAT_CAP_OBJ_SERVICE,
        0,
        required_rights,
        &required_scope,
        &vr);

    if (st != BHARAT_CAP_OK || !vr.allowed) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    return BHARAT_IPC_STATUS_OK;
}
